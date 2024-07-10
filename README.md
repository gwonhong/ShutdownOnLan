# shutdown-on-3-wol

## Overview

`shutdown-on-3-wol` is a simple server that listens for magic packets and shuts down the PC if it receives three packets in a row within a 3-second interval.

## Files

- `src/server.c`: The C source code of the server.
- `scripts/run_silently.bat`: The batch script to run the executable silently. This script assumes the executable located at `../bin/`. You can modify it on your own.

## Usage

1. **Download the bundle:**

   Download the latest version of the bundle (ZIP file) from the [Releases](https://github.com/gwonhong/shutdown-on-3-wol/releases) tab.

2. **Extract the bundle:**

   Extract the ZIP file. It contains `server.exe` and `run_silently.bat`. Make sure they are in the same folder. If you need to separate them, you need to edit the `run_silently.bat` to indicate the `server.exe` properly.

3. **Register the script with Windows Task Scheduler to run on startup:**

   - Open Task Scheduler and create a new task.
   - **In the "General" tab, provide a name for the task and check "Run with the highest privileges."** This server needs administrative privileges.
   - In the "Triggers" tab, create a new trigger and set it to begin the task "At startup."
   - In the "Actions" tab, create a new action to start a program and browse to select `run_silently.bat` in the `scripts` directory.
   - In the "Conditions" tab, uncheck "Start the task only if the computer is on AC power" if you want it to run on battery as well.
   - In the "Settings" tab, ensure "Allow task to be run on demand" is checked.

4. **Send magic packets to trigger shutdown:**

   - Use any tool that can send a magic packet to your PC's MAC address on **port 9**.
   - Send the magic packet **three times in a row within a 3-second interval** to shut down your PC.

## My Use Case

I use a combination of the iOS `Wake Me Up` application and the iOS `Shortcuts` app to send three magic packets in a row. This method is convenient and reliable for iPhone users. I highly recommend this setup if you use an iPhone.

## To Compile by yourself

1. **Clone this repository:**

   ```sh
   git clone https://github.com/yourusername/shutdown-on-3-wol.git
   cd shutdown-on-3-wol
   ```

2. **Compile the source code:**

   Ensure you have GCC installed. Then, compile the executable using the following command:
   
   ```sh
   gcc -o ./bin/server ./src/server.c -lws2_32 -liphlpapi "-Wl,-subsystem,console"
   ```

   This will create the executable in the `bin` directory.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributions

Feel free to fork this project, make improvements, and send pull requests. Contributions are welcome!

## Disclaimer

Use this tool responsibly. Shutting down a PC remotely can disrupt work and potentially cause data loss if unsaved work is present.
