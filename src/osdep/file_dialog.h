#pragma once

#include <string>

void file_dialog_init();
void file_dialog_shutdown();

void OpenFileDialogKey(const char* key, const char* title, const char* filters, const std::string& initialPath, bool saveMode = false);
bool ConsumeFileDialogResultKey(const char* key, std::string& outPath);
bool IsFileDialogOpenKey(const char* key);

void OpenDirDialogKey(const char* key, const std::string& initialPath);
bool ConsumeDirDialogResultKey(const char* key, std::string& outPath);
bool IsDirDialogOpenKey(const char* key);

void OpenFileDialog(const char* title, const char* filters, const std::string& initialPath);
bool ConsumeFileDialogResult(std::string& outPath);
bool IsFileDialogOpen();
void OpenDirDialog(const std::string& initialPath);
bool ConsumeDirDialogResult(std::string& outPath);
bool IsDirDialogOpen();
