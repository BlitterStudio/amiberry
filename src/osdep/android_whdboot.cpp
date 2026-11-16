// The launcher's WHDBoot downloader. It runs in the launcher process, which is
// separate from AmiberryActivity's :sdl process, so it never shares zfile state
// with a running emulator.
#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "target.h"
#include "settings/amiberry_whdboot.h"

#include <jni.h>

#include <atomic>
#include <string>
#include <vector>

namespace {

std::atomic_flag whdboot_download_running = ATOMIC_FLAG_INIT;

std::string from_java(JNIEnv* env, jstring text)
{
	if (text == nullptr)
		return {};
	const char* chars = env->GetStringUTFChars(text, nullptr);
	std::string result = chars ? chars : "";
	if (chars)
		env->ReleaseStringUTFChars(text, chars);
	return result;
}

jobjectArray to_java(JNIEnv* env, const std::vector<std::string>& values)
{
	auto string_class = env->FindClass("java/lang/String");
	auto result = env->NewObjectArray(static_cast<jsize>(values.size()), string_class, nullptr);
	for (size_t index = 0; index < values.size(); ++index) {
		auto value = env->NewStringUTF(values[index].c_str());
		env->SetObjectArrayElement(result, static_cast<jsize>(index), value);
		env->DeleteLocalRef(value);
	}
	env->DeleteLocalRef(string_class);
	return result;
}

void throw_status_exception(JNIEnv* env)
{
	auto exception_class = env->FindClass("java/lang/RuntimeException");
	if (exception_class != nullptr) {
		env->ThrowNew(exception_class, "Could not read WHDBoot status");
		env->DeleteLocalRef(exception_class);
	}
}

void configure_whdboot(const std::string& path, const jboolean use_jst)
{
	// Kotlin resolves amiberry.conf with the same managed-path rules the core
	// uses. The downloader only needs the effective WHDBoot path and this
	// selection; starting amiberry_main() here would initialise SDL in the
	// launcher process and conflict with the dedicated emulator process.
	set_whdbootpath(path);
	amiberry_options.use_jst_instead_of_whd = use_jst == JNI_TRUE;
}

struct progress_callback {
	JavaVM* jvm;
	jobject listener;
	jmethodID on_progress;
};

int report_progress(const amiberry_whdboot_progress* progress, void* data)
{
	auto* callback = static_cast<progress_callback*>(data);
	JNIEnv* env = nullptr;
	bool attached = false;
	if (callback->jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
		if (callback->jvm->AttachCurrentThread(&env, nullptr) != JNI_OK)
			return 1;
		attached = true;
	}
	auto label = env->NewStringUTF(progress->label ? progress->label : "");
	const auto cancel = env->CallBooleanMethod(callback->listener, callback->on_progress,
		progress->step, progress->total_steps, label,
		static_cast<jlong>(progress->bytes_downloaded), static_cast<jlong>(progress->bytes_total));
	env->DeleteLocalRef(label);
	const bool failed = env->ExceptionCheck();
	if (failed)
		env->ExceptionClear();
	if (attached)
		callback->jvm->DetachCurrentThread();
	return failed || cancel == JNI_TRUE;
}

} // namespace


extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_blitterstudio_amiberry_data_WhdbootTools_nativeStatus(JNIEnv* env, jobject, jstring path, jboolean)
{
	try {
		const auto root = from_java(env, path);
		amiberry_whdboot_status status{};
		amiberry_whdboot_status_get_at_path(root.c_str(), &status);
		return to_java(env, {
			status.whdload_installed ? "1" : "0",
			status.jst_installed ? "1" : "0",
			status.rtb_files_installed ? "1" : "0",
			status.libraries_installed ? "1" : "0",
			status.downloads_available ? "1" : "0"
		});
	} catch (...) {
		throw_status_exception(env);
		return nullptr;
	}
}

extern "C" JNIEXPORT jint JNICALL
Java_com_blitterstudio_amiberry_data_WhdbootTools_nativeDownload(JNIEnv* env, jobject, jstring path, jboolean use_jst,
	jobject listener)
{
	if (whdboot_download_running.test_and_set())
		return AMIBERRY_WHDBOOT_DOWNLOAD_BUSY;
	struct running_download {
		~running_download() { whdboot_download_running.clear(); }
	} clear_running;

	jobject listener_ref = nullptr;
	try {
		JavaVM* jvm = nullptr;
		if (env->GetJavaVM(&jvm) != JNI_OK)
			return AMIBERRY_WHDBOOT_DOWNLOAD_FAILED;
		configure_whdboot(from_java(env, path), use_jst);
		auto listener_class = env->GetObjectClass(listener);
		auto on_progress = env->GetMethodID(listener_class, "onProgress", "(IILjava/lang/String;JJ)Z");
		env->DeleteLocalRef(listener_class);
		if (on_progress == nullptr)
			return AMIBERRY_WHDBOOT_DOWNLOAD_FAILED;
		listener_ref = env->NewGlobalRef(listener);
		if (listener_ref == nullptr)
			return AMIBERRY_WHDBOOT_DOWNLOAD_FAILED;
		progress_callback callback{jvm, listener_ref, on_progress};
		const auto result = amiberry_whdboot_download(report_progress, &callback);
		env->DeleteGlobalRef(listener_ref);
		return result;
	} catch (...) {
		if (listener_ref != nullptr)
			env->DeleteGlobalRef(listener_ref);
		return AMIBERRY_WHDBOOT_DOWNLOAD_FAILED;
	}
}
