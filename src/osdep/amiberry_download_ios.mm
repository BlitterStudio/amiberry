// download_file() for the iOS build, which has no libcurl: an NSURLSession
// download with the same contract the curl implementation has — progress
// reporting, cancellation, an optional .bak backup and an atomic replace
// of the destination (the file is written to <destination>.tmp and renamed
// only after the download completed).

#if defined(AMIBERRY_IOS) && !defined(AMIBERRY_HAS_CURL)

// Amiberry's headers are deliberately not included here: their BOOL
// typedef clashes with Objective-C's (see macos_window.mm, which likewise
// keeps to system frameworks). What is needed is declared instead.
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string>

#import <Foundation/Foundation.h>

void write_log(const char* format, ...);
bool download_file(const std::string& source, const std::string& destination, bool keep_backup);
bool download_file(const std::string& source, const std::string& destination, bool keep_backup,
	const std::function<bool(int64_t, int64_t)>& progress_cb, std::atomic<bool>* cancel_flag);

namespace
{

struct ios_download_state
{
	FILE* output = nullptr;
	int64_t received = 0;
	int64_t total = 0;
	bool failed = false;
	bool cancelled = false;
	bool done = false;
	dispatch_semaphore_t completion = nullptr;
};

}

@interface AmiberryDownloadDelegate : NSObject <NSURLSessionDataDelegate>
@property (nonatomic, assign) ios_download_state* state;
@property (nonatomic, assign) const std::function<bool(int64_t, int64_t)>* progress;
@property (nonatomic, assign) std::atomic<bool>* cancelFlag;
@end

@implementation AmiberryDownloadDelegate

- (void)URLSession:(NSURLSession*)session dataTask:(NSURLSessionDataTask*)dataTask
	didReceiveResponse:(NSURLResponse*)response
	completionHandler:(void (^)(NSURLSessionResponseDisposition))completionHandler
{
	if ([response isKindOfClass:[NSHTTPURLResponse class]]
		&& ((NSHTTPURLResponse*)response).statusCode >= 400)
	{
		write_log("Download failed: HTTP status %ld (%s)\n",
			static_cast<long>(((NSHTTPURLResponse*)response).statusCode),
			dataTask.currentRequest.URL.absoluteString.UTF8String);
		self.state->failed = true;
		completionHandler(NSURLSessionResponseCancel);
		return;
	}
	self.state->total = response.expectedContentLength > 0 ? response.expectedContentLength : 0;
	completionHandler(NSURLSessionResponseAllow);
}

- (void)URLSession:(NSURLSession*)session dataTask:(NSURLSessionDataTask*)dataTask didReceiveData:(NSData*)data
{
	if (self.state->failed || self.state->cancelled)
		return;
	if (self.state->output == nullptr || fwrite(data.bytes, 1, data.length, self.state->output) != data.length)
	{
		self.state->failed = true;
		[dataTask cancel];
		return;
	}
	self.state->received += static_cast<int64_t>(data.length);

	if (self.cancelFlag != nullptr && self.cancelFlag->load())
	{
		self.state->cancelled = true;
		[dataTask cancel];
		return;
	}
	if (self.progress != nullptr && (*self.progress)(self.state->received, self.state->total))
	{
		self.state->cancelled = true;
		[dataTask cancel];
	}
}

- (void)URLSession:(NSURLSession*)session task:(NSURLSessionTask*)task didCompleteWithError:(NSError*)error
{
	if (error != nullptr && self.state->cancelled == false)
	{
		self.state->failed = true;
		write_log("Download failed: %s (URL: %s)\n",
			error.localizedDescription.UTF8String, task.currentRequest.URL.absoluteString.UTF8String);
	}
	self.state->done = true;
	dispatch_semaphore_signal(self.state->completion);
}

@end

namespace
{

bool download_file_ios(const std::string& source, const std::string& destination, const bool keep_backup,
	const std::function<bool(int64_t, int64_t)>* progress_cb, std::atomic<bool>* cancel_flag)
{
	@autoreleasepool
	{
		const std::string temp_path = destination + ".tmp";
		if (FILE* existing = fopen(temp_path.c_str(), "rb"))
		{
			fclose(existing);
			std::remove(temp_path.c_str());
		}

		FILE* fp = fopen(temp_path.c_str(), "wb");
		if (!fp)
		{
			write_log("Failed to open temporary file for writing: %s\n", temp_path.c_str());
			return false;
		}

		ios_download_state state;
		state.output = fp;
		state.completion = dispatch_semaphore_create(0);

		AmiberryDownloadDelegate* delegate = [[AmiberryDownloadDelegate alloc] init];
		delegate.state = &state;
		delegate.progress = progress_cb;
		delegate.cancelFlag = cancel_flag;

		NSURLSessionConfiguration* configuration = [NSURLSessionConfiguration ephemeralSessionConfiguration];
		configuration.timeoutIntervalForRequest = 15;
		configuration.timeoutIntervalForResource = 120;
		NSURLSession* session = [NSURLSession sessionWithConfiguration:configuration
			delegate:delegate delegateQueue:nil];

		NSURLSessionDataTask* task = [session dataTaskWithURL:
			[NSURL URLWithString:[NSString stringWithUTF8String:source.c_str()]]];
		write_log("Downloading %s ...\n", destination.c_str());
		[task resume];

		dispatch_semaphore_wait(state.completion, DISPATCH_TIME_FOREVER);
		[session invalidateAndCancel];
		dispatch_release(state.completion); // MRC: the semaphore was created retained
		fclose(fp);
		state.output = nullptr;
		[delegate release];

		if (state.cancelled)
		{
			write_log("Download cancelled (URL: %s)\n", source.c_str());
			std::remove(temp_path.c_str());
			return false;
		}
		if (state.failed)
		{
			std::remove(temp_path.c_str());
			return false;
		}

		if (keep_backup)
		{
			if (FILE* previous = fopen(destination.c_str(), "rb"))
			{
				fclose(previous);
				const auto dot_pos = destination.find_last_of('.');
				const std::string backup_path = (dot_pos != std::string::npos)
					? destination.substr(0, dot_pos).append(".bak")
					: destination + ".bak";
				std::remove(backup_path.c_str());
				if (std::rename(destination.c_str(), backup_path.c_str()) < 0)
					write_log("%s\n", strerror(errno));
			}
		}

		write_log("Renaming downloaded temporary file %s to final destination\n", temp_path.c_str());
		if (std::rename(temp_path.c_str(), destination.c_str()) < 0)
		{
			write_log("%s\n", strerror(errno));
			std::remove(temp_path.c_str());
			return false;
		}
		return true;
	}
}

}

bool download_file(const std::string& source, const std::string& destination, bool keep_backup)
{
	return download_file_ios(source, destination, keep_backup, nullptr, nullptr);
}

bool download_file(const std::string& source, const std::string& destination, bool keep_backup,
	const std::function<bool(int64_t, int64_t)>& progress_cb, std::atomic<bool>* cancel_flag)
{
	return download_file_ios(source, destination, keep_backup, &progress_cb, cancel_flag);
}

#endif // AMIBERRY_IOS && !AMIBERRY_HAS_CURL
