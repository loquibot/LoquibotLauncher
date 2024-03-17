#include <sdkddkver.h>
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING

#define CURL_STATICLIB
#define UNICODE
#define _UNICODE

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <tchar.h>
#include <string> 
#include <CommCtrl.h>
#include <Urlmon.h>
#include <curl\curl.h>
#include <experimental/filesystem>
#include "zip.h"
#include "unzip.h"
#include <fstream>
#include <streambuf>
#include <iostream>
#include <thread>

namespace fs = std::experimental::filesystem;

#pragma comment(lib, "comctl32.lib")

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define ID_DEFAULTPROGRESSCTRL	401
#define ID_SMOOTHPROGRESSCTRL	402
#define ID_VERTICALPROGRESSCTRL	403

void updateProgressBar(int value);
std::string readToString(std::string url);
void createDirectory(fs::path path);
void unzipFile(fs::path path, fs::path destination);
std::string readFileToString(fs::path path);
void writeToFile(fs::path path, std::string text);
std::vector<std::string> split(std::string s, std::string delimiter);
size_t writeData(void* ptr, size_t size, size_t nmemb, FILE* stream);
VOID startup(LPCTSTR lpApplicationName);

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

RECT rect;
int centerWindow(HWND parent_window, int width, int height)
{
	GetClientRect(parent_window, &rect);
	rect.left = (rect.right / 2) - (width / 2);
	rect.top = (rect.bottom / 2) - (height / 2);
	return 0;
}

int progressFunction(void* ptr, double TotalToDownload, double NowDownloaded, double TotalToUpload, double NowUploaded)
{
	if (NowDownloaded <= 0 || TotalToDownload <= 0.0) {
		updateProgressBar(0);
		return 0;
	}
	
	double percentDownloaded = (NowDownloaded / TotalToDownload) * 100;

	updateProgressBar(percentDownloaded);
	
	return 0;
}
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}
size_t writeDownloadToFile(void* ptr, size_t size, size_t nmemb, void* stream)
{
	size_t written = fwrite(ptr, size, nmemb, (FILE*)stream);
	return written;
}


HWND hSmoothProgressCtrl;
HWND hWnd;

std::string titleText;

fs::path createPath(std::string pathAfter) {

	char* appdata = std::getenv("APPDATA");

	fs::path path(appdata);

	path.append(pathAfter);

	return path;
}

void runDownload() {

	fs::path loquibotPath = createPath("\\loquibot");
	fs::path jrePath = createPath("\\loquibot\\jre");
	fs::path jreZipPath = createPath("\\loquibot\\jre.zip");
	fs::path versionPath = createPath("\\loquibot\\version.txt");
	fs::path binPath = createPath("\\loquibot\\bin");
	fs::path exePath = createPath("\\loquibot\\bin\\board.exe");

	std::string version = readToString("http://164.152.25.111:35424/Loquibot/.version");

	std::string oldVersion;

	if (!fs::exists(versionPath)) {
		oldVersion = "version=0";
	}
	else {
		oldVersion = readFileToString(versionPath);
	}

	std::vector<std::string> versionSplit = split(version, "=");
	std::vector<std::string> oldVersionSplit = split(oldVersion, "=");

	double dVersion = std::stod(versionSplit.at(1));
	double dOldVersion = std::stod(oldVersionSplit.at(1));

	bool update = dVersion > dOldVersion;
	createDirectory(loquibotPath);
	createDirectory(binPath);

	writeToFile(versionPath, version);


	if (!fs::is_directory(jrePath) || !fs::exists(jrePath)) {

		createDirectory(jrePath);

		CURL* curl;
		CURLcode res;
		std::string url = "http://164.152.25.111:35424/Loquibot/jre.zip";
		titleText = "loquibot - Installing Java";
		curl = curl_easy_init();
		if (curl)
		{
			std::ofstream fp(jreZipPath, std::ios::binary);

			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeDownloadToFile);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &fp);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progressFunction);
			res = curl_easy_perform(curl);

			if (fp) fp.close();

			curl_easy_cleanup(curl);

		}

		unzipFile(jreZipPath, jrePath);
		fs::remove(jreZipPath);
	}


	if (update) {
		CURL* curl2;
		CURLcode res2;
		std::string url2 = "http://164.152.25.111:35424/Loquibot/board.exe";
		titleText = "loquibot - Updating";
		updateProgressBar(0);

		curl2 = curl_easy_init();
		if (curl2)
		{
			std::ofstream fp(exePath, std::ios::binary);

			curl_easy_setopt(curl2, CURLOPT_URL, url2.c_str());
			curl_easy_setopt(curl2, CURLOPT_WRITEFUNCTION, writeDownloadToFile);
			curl_easy_setopt(curl2, CURLOPT_WRITEDATA, &fp);
			curl_easy_setopt(curl2, CURLOPT_NOPROGRESS, 0L);
			curl_easy_setopt(curl2, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl2, CURLOPT_PROGRESSFUNCTION, progressFunction);
			res2 = curl_easy_perform(curl2);


			if (fp) fp.close();

			curl_easy_cleanup(curl2);

		}
	}
	//End download code

	startup(exePath.c_str());

	PostMessage(hWnd, WM_CLOSE, 0, 0);
}


int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd) 
{
	std::locale::global(std::locale{ ".utf-8" });

	INITCOMMONCONTROLSEX iccex;
	iccex.dwSize = sizeof(iccex);
	iccex.dwICC = ICC_PROGRESS_CLASS;

	if (!InitCommonControlsEx(&iccex)) return 1;

	WNDCLASSEX windowClassEx;
	ZeroMemory(&windowClassEx, sizeof(windowClassEx));

	windowClassEx.cbSize = sizeof(windowClassEx);
	windowClassEx.style = CS_HREDRAW | CS_VREDRAW;
	windowClassEx.lpszClassName = TEXT("LOQUIBOT");
	windowClassEx.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	windowClassEx.hCursor = LoadCursor(hInstance, IDC_ARROW);
	windowClassEx.lpfnWndProc = WndProc;
	windowClassEx.hInstance = hInstance;

	if (!RegisterClassEx(&windowClassEx)) return 1;

	centerWindow(GetDesktopWindow(), 450, 62);
	
	hWnd = CreateWindowEx(0, TEXT("LOQUIBOT"), TEXT("Loquibot"),
		WS_OVERLAPPED | WS_MINIMIZEBOX, rect.left, rect.top, 450, 62,
		NULL, NULL, hInstance, NULL);

	if (!hWnd) return 1;

	hSmoothProgressCtrl = CreateWindowEx(0, PROGRESS_CLASS, TEXT(""),
		WS_CHILD | WS_VISIBLE | PBS_SMOOTH, -2, -2, 450, 30,
		hWnd, (HMENU)ID_SMOOTHPROGRESSCTRL, hInstance, NULL);

	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);

	//Download dode here

	std::thread thread_obj(runDownload);
	
	thread_obj.detach();

	MSG msg;
	while (GetMessage(&msg, hWnd, 0, 0) > 0) {
		DispatchMessage(&msg);
	}

	UnregisterClass(windowClassEx.lpszClassName, hInstance);

	return (int)msg.wParam;
}

void updateProgressBar(int value) 
{
	std::string title = titleText + " " + std::to_string(value) + "%";
	std::wstring wTitle = std::wstring(title.begin(), title.end());

	SendMessage(hSmoothProgressCtrl, PBM_SETPOS, (WPARAM)(INT)value, 0);
	SetWindowText(hWnd, wTitle.c_str());
}

std::string readToString(std::string url) 
{

	CURL* curl;
	CURLcode res;
	std::string readBuffer;

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

	}
	return readBuffer;

}

void createDirectory(fs::path path) 
{
	try {
		if (!fs::is_directory(path) || !fs::exists(path)) {
			fs::create_directory(path);
		}
	}
	catch (std::exception& e) {
		MessageBoxA(NULL, e.what(), "Exception", 0);
	}
}

void unzipFile(fs::path path, fs::path destination) 
{
	
	HZIP hz; DWORD writ;
	std::wstring wPath = std::wstring(path.u8string().begin(), path.u8string().end());
	std::wstring wDest = std::wstring(destination.u8string().begin(), destination.u8string().end());

	hz = OpenZip(wPath.c_str(), 0);
	SetUnzipBaseDir(hz, wDest.c_str());
	ZIPENTRY ze;
	GetZipItem(hz, -1, &ze);
	int numitems = ze.index;
	for (int zi = 0; zi < numitems; zi++)
	{
		GetZipItem(hz, zi, &ze);
		UnzipItem(hz, zi, ze.name);
	}
	CloseZip(hz);

}

std::string readFileToString(fs::path path) 
{
	std::ifstream t(path);
	std::string str((std::istreambuf_iterator<char>(t)),
		std::istreambuf_iterator<char>());
	return str;
}

void writeToFile(fs::path path, std::string text)
{
	std::ofstream outfile(path);
	outfile << text;
	outfile.close();

}
std::vector<std::string> split(std::string s, std::string delimiter) 
{
	size_t pos_start = 0, pos_end, delim_len = delimiter.length();
	std::string token;
	std::vector<std::string> res;

	while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) 
	{
		token = s.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		res.push_back(token);
	}

	res.push_back(s.substr(pos_start));
	return res;
}
size_t writeData(void* ptr, size_t size, size_t nmemb, FILE* stream) 
{
	size_t written = fwrite(ptr, size, nmemb, stream);
	return written;
}
VOID startup(LPCTSTR lpApplicationName) 
{
	
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	CreateProcess(lpApplicationName, 
		NULL,       
		NULL,          
		NULL,      
		FALSE,    
		0,            
		NULL,          
		NULL,     
		&si,          
		&pi         
	);
	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}
