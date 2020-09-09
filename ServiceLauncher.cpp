#include <iostream>
#include <csignal>
#include <stdexcept>
#include <string>
#include <chrono>
#include "include/ServiceController.h"

using namespace std;
using namespace std::this_thread;
using namespace std::chrono;

/// <summary>
/// Amount of time waiting for service stops.
/// </summary>
long long secondsWaiting = 0;
/// <summary>
/// Service's name.
/// </summary>
const char* service_name = NULL;
/// <summary>
/// Amount of time for thread sleeping.
/// </summary>
int sleepSeconds = 0;
/// <summary>
/// ServiceController instance that was initialized.
/// </summary>
ServiceController* local_service = nullptr;

/// <summary>
/// Executes final actions. 
/// </summary>
void execute_exit() {
	if (secondsWaiting > 0)
	{
		cout << "The launcher waits " << secondsWaiting << " second(s)." << endl;
	}

	if (local_service != nullptr && local_service->GetStatus() == ServiceStatus::Running) {
		ServiceController* service = local_service;
		local_service = nullptr;
		if (sleepSeconds > 0) {
			service->Stop();
			cout << "Service [" << service_name << "] is stopping..." << endl;
			service->WaitForStatus(ServiceStatus::Stopped);
		}
		free(local_service);
	}
}

/// <summary>
/// Interrupt signal handle.
/// </summary>
/// <param name="signum">signal code.</param>
void signal_handler(int signum) {
	cout << "Interrupt signal (" << signum << ") received." << endl;
	execute_exit();
	exit(signum);
}

/// <summary>
/// Prints service's name error argument. 
/// </summary>
/// <returns>Error code.</returns>
int print_service_name_error() {
	cout << "Please specify the service name." << endl;
	return -1;
}

/// <summary>
/// Prints sleep time error argument. 
/// </summary>
/// <returns>Error code.</returns>
int print_sleep_time_error() {
	cout << "Sleep time must be zero or an positive integer." << endl;
	return -2;
}

/// <summary>
/// Tries to get an integer from char pointer.
/// </summary>
/// <param name="text">char pointer.</param>
/// <param name="value">pointer to value.</param>
/// <returns>Whether or not integer was gotten.</returns>
bool try_get_number(char* text, int* value) {
	if (text != NULL && text != "") {
		try {
			*value = stoi(string(text));
		}
		catch (const invalid_argument& ia) {
			return false;
		}
	}
	else {
		value = 0;
	}

	return true;
}

/// <summary>
/// Converts a char pointer to wide char pointer.
/// </summary>
/// <param name="c">char pointer.</param>
/// <returns>wide char pointer.</returns>
const wchar_t* get_widechar_ptr(const char* c) {
	const size_t cSize = strlen(c) + 1;
	wchar_t* wc = new wchar_t[cSize];
	mbstowcs(wc, c, cSize);
	return wc;
}

/// <summary>
/// Executes the service start and wait actions.
/// </summary>
/// <returns>Error code.</returns>
int launch_wait_service() {
	auto service = ServiceController{ get_widechar_ptr(service_name) };

	cout << "Service [" << service_name << "] Status: " << (int)service.GetStatus() << endl;

	if (service.GetStatus() == ServiceStatus::Stopped &&
		service.GetStatus() != ServiceStatus::Starting) {
		local_service = &service;
		service.Start();
		cout << "Service [" << service_name << "] is starting..." << endl;
	}

	service.WaitForStatus(ServiceStatus::Running);
	if (service.GetStatus() != ServiceStatus::Running) {
		cout << "Service [" << service_name << "] isn't running." << endl;
		return -3;
	}
	else if (sleepSeconds > 0) {
		cout << "The launcher will wait until Service [" << service_name << "] stops." << endl;
		while (service.GetStatus() == ServiceStatus::Running) {
			sleep_for(seconds(sleepSeconds));
			secondsWaiting++;
		}
		execute_exit();
	}

	return 0;
}

/// <summary>
/// Executes the service stops actions.
/// </summary>
/// <returns>Error code.</returns>
int launch_stop_service() {
	auto service = ServiceController{ get_widechar_ptr(service_name) };

	cout << "Service [" << service_name << "] Status: " << (int)service.GetStatus() << endl;

	if (service.GetStatus() != ServiceStatus::Stopped) {
		local_service = &service;
		service.Start();
		cout << "Service [" << service_name << "] is stopping..." << endl;
		service.WaitForStatus(ServiceStatus::Stopped);
	}

	return 0;
}

/// <summary>
/// Main method.
/// </summary>
/// <param name="argc">Arguments counter.</param>
/// <param name="argv">Arguments vecto.</param>
/// <returns>Exit code.</returns>
int main(int argc, char* argv[]) {
	signal(SIGINT, signal_handler);
	int result = 0;

	if (argc >= 2) {
		if (argv[0] != "") {
			service_name = argv[1];
			if (argc > 2 && !try_get_number(argv[2], &sleepSeconds)) {
				result = print_sleep_time_error();
			}
			else {
				if (sleepSeconds >= 0) {
					result = launch_wait_service();
				}
				else {
					result = launch_stop_service();
				}
			}
		}
		else {
			result = print_service_name_error();
		}
	}
	else {
		result = print_service_name_error();
	}

	return result;
}