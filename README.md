# profiler-lateral-movement
Lateral Movement via the .NET Profiler

Pretty much all of this is code that has been cobbled together from elsewhere, most notably:

- [Pavel Yosifovich's presentation and example code for a .NET profiler](https://github.com/zodiacon/DotNextMoscow2019) - The profiling payload is 99% his work from this repository

- [Yaxser's WMI lateral movement BOF](https://github.com/Yaxser/CobaltStrike-BOF/tree/master/WMI%20Lateral%20Movement) - The WMI bof for creating a process with the appropriate environment variables is 99% their work from this repository

To build, open each of the solution files in their respective folders, and build for release x64 - the console output will print the path to the built binaries.

A prebuilt object file and the cna script already exist in the wmi BOF folder.
