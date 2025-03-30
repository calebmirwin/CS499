"""
smartThermostatRemote.py

Designer: Caleb Irwin
Date: 03/23/2025

Description:
    Python Remote to communicate and control smart thermostat via TCP/IP.
    This script creates a GUI using Tkinter to display the current temperature,
    setpoint, and heat status. The user can adjust the setpoint and choose to
    view/edit a schedule with time-based setpoints. The remote communicates
    over sockets, sending commands and receiving updates. Real-time GUI updates
    reflect thermostat state.
"""

import tkinter as tk
from tkinter import ttk
from tkinter import Canvas
import socket
import threading

THERMOSTAT_IP = "192.168.50.92"
THERMOSTAT_PORT = 5000


class ThermostatGUI:
    """
    GUI class for Smart Thermostat Remote Control.

    Provides:
        - Real-time temperature and heating status display
        - Manual setpoint adjustment
        - Schedule editing with two daily entries
        - Socket communication with physical thermostat
    """
    def __init__(self, root):
        """
        Initializes the GUI and starts the socket thread.
        """

        self.root = root
        self.root.title("Smart Thermostat Remote")

        self.temperature = tk.IntVar(value=0)
        self.setpoint = tk.IntVar(value=20)
        self.pending_setpoint = tk.IntVar(value=20)
        self.heat = tk.BooleanVar(value=False)
        self.in_main_view = True
        self.waiting_for_ack = False

        self.setpoint_label = None
        

        self.sock = None
        self.running = True

        self.create_main_view()
        threading.Thread(target=self.socket_loop, daemon=True).start()

    def create_main_view(self):
        """
        Constructs the main screen of the GUI.
        Shows current temperature, heat status indicator, setpoint control buttons, and a schedule button.
        """
            
        self.in_main_view = True
        for widget in self.root.winfo_children():
            widget.destroy()

        # Row 0: Current Temp + Heat Indicator
        ttk.Label(self.root, text="Current Temp:", font=("Arial", 14)).grid(row=0, column=0, sticky="e", padx=5, pady=5)
        ttk.Label(self.root, textvariable=self.temperature, font=("Arial", 14)).grid(row=0, column=1, sticky="w", padx=5)

        ttk.Label(self.root, text="HEAT ON:", font=("Arial", 14)).grid(row=0, column=2, sticky="e", padx=(0, 0))
        self.heat_canvas = Canvas(self.root, width=20, height=20, highlightthickness=0)
        self.heat_canvas.grid(row=0, column=3, padx=(0, 10))
        self.heat_indicator = self.heat_canvas.create_oval(4, 4, 16, 16, fill="gray")

        # Row 1: Setpoint
        ttk.Label(self.root, text="Setpoint:", font=("Arial", 14)).grid(row=1, column=0, sticky="e", padx=5, pady=5)
        self.setpoint_label = ttk.Label(self.root, textvariable=self.pending_setpoint, font=("Arial", 14))
        self.setpoint_label.grid(row=1, column=1, sticky="w", padx=5)

        # Row 2: Setpoint UP, DOWN, Update Buttons
        ttk.Button(self.root, text="\u25BC", command=self.decrease_setpoint).grid(row=2, column=0, pady=10)
        ttk.Button(self.root, text="\u25B2", command=self.increase_setpoint).grid(row=2, column=1, pady=10)
        ttk.Button(self.root, text="Update Setpoint", command=self.send_pending_setpoint).grid(row=2, column=2, columnspan=2, pady=10, padx = (25))

        # Row 3: Schedule Button
        ttk.Button(self.root, text="Set Schedule", command=self.create_schedule_view).grid(row=3, column=0, columnspan=4, pady=(5), padx=(0, 9))

    def create_schedule_view(self):
        """
        Switches to schedule editing screen of the GUI.
        Sends GETSCHEDULE request to fetch current schedule from thermostat and updates the GUI accordingly.
        Displays two schedule entries and allows time and setpoint adjustment.
        """

        self.in_main_view = False
        self.send_command("GETSCHEDULE")  
        for widget in self.root.winfo_children():
            widget.destroy()

        self.schedule = [
            {"hour": tk.IntVar(value=8), "minute": tk.IntVar(value=0), "setpoint": tk.IntVar(value=20)},
            {"hour": tk.IntVar(value=20), "minute": tk.IntVar(value=0), "setpoint": tk.IntVar(value=18)}
        ]

        def create_schedule_row(parent, row, entry):
            # Create a row of widgets for one schedule entry (time and setpoint)
            base_row = row * 4

            # Entry
            ttk.Label(parent, text=f"Entry {row + 1}").grid(row=base_row, column=0, columnspan=1, pady=(10, 0), padx=(10))

            # Time 
            ttk.Label(parent, text="Time:").grid(row=base_row+1, column=0, padx=(20, 0))
            ttk.Label(parent, textvariable=entry["hour"]).grid(row=base_row+1, column=1)
            ttk.Label(parent, text=":").grid(row=base_row+1, column=2)
            ttk.Label(parent, textvariable=entry["minute"]).grid(row=base_row+1, column=3)

            # Setpoint
            ttk.Label(parent, text="Setpoint:").grid(row=base_row+1, column=4, padx=(10, 0))
            ttk.Label(parent, textvariable=entry["setpoint"]).grid(row=base_row+1, column=5)

            # Controls
            ttk.Button(parent, text="▲", command=lambda: entry["hour"].set((entry["hour"].get() + 1) % 24)).grid(row=base_row+2, column=1)
            ttk.Button(parent, text="▼", command=lambda: entry["hour"].set((entry["hour"].get() - 1) % 24)).grid(row=base_row+3, column=1)

            ttk.Button(parent, text="▲", command=lambda: entry["minute"].set((entry["minute"].get() + 1) % 60)).grid(row=base_row+2, column=3)
            ttk.Button(parent, text="▼", command=lambda: entry["minute"].set((entry["minute"].get() - 1) % 60)).grid(row=base_row+3, column=3)

            ttk.Button(parent, text="▲", command=lambda: entry["setpoint"].set(min(entry["setpoint"].get() + 1, 99))).grid(row=base_row+2, column=5)
            ttk.Button(parent, text="▼", command=lambda: entry["setpoint"].set(max(entry["setpoint"].get() - 1, 0))).grid(row=base_row+3, column=5)

        for i, entry in enumerate(self.schedule):
            create_schedule_row(self.root, i, entry)

        def submit_schedule():
            s1 = self.schedule[0]
            s2 = self.schedule[1]
            schedule_str = f"SCHEDULE:[{s1['hour'].get():02d}:{s1['minute'].get():02d},{s1['setpoint'].get()}]," + \
                           f"[{s2['hour'].get():02d}:{s2['minute'].get():02d},{s2['setpoint'].get()}]"
            self.send_command(schedule_str)

        ttk.Button(self.root, text="Submit Schedule", command=submit_schedule).grid(row=10, column=0, columnspan=6, pady=10)
        ttk.Button(self.root, text="Back", command=self.create_main_view).grid(row=11, column=0, columnspan=6, pady=(0, 10))

    def update_heat_indicator(self):
        """
        Updates the heat indicator circle based on current heat status received from thermostat.
        """
        
        color = "red" if self.heat.get() else "gray"
        try:
            if self.heat_canvas.winfo_exists(): # Only update if the canvas still exists (On the Main View)
                self.heat_canvas.itemconfig(self.heat_indicator, fill=color)
        except tk.TclError:
            pass

    def increase_setpoint(self):
        """
        Increases the pending setpoint by 1 (up to 99).
        Marks value as pending with orange text until confirmed from thermostat.
        """

        self.waiting_for_ack = True
        val = self.pending_setpoint.get()
        if val < 99:
            self.pending_setpoint.set(val + 1)
            self.setpoint_label.config(foreground="orange")

    def decrease_setpoint(self):
        """
        Decreases the pending setpoint by 1 (up to 99).
        Marks value as pending with orange text until confirmed from thermostat.
        """

        self.waiting_for_ack = True
        val = self.pending_setpoint.get()
        if val > 0:
            self.pending_setpoint.set(val - 1)
            self.setpoint_label.config(foreground="orange")

    def send_pending_setpoint(self):
        """
        Sends current pending setpoint to thermostat.
        Resets setpoint indicator to normal text color.
        """

        self.waiting_for_ack = True
        self.send_command(f"SETPOINT:{self.pending_setpoint.get()}")
        self.setpoint_label.config(foreground="black")


    def socket_loop(self):
        """
        Threaded socket communication loop.
        Connects to thermostat and listens for incoming messages.
        Handles each line of data received.
        """

        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((THERMOSTAT_IP, THERMOSTAT_PORT))

            buffer = ""
            while self.running:
                chunk = self.sock.recv(1024).decode()
                buffer += chunk

                while '\n' in buffer:
                    line, buffer = buffer.split('\n', 1)
                    self.handle_message(line.strip())

        except Exception as e:
            print("Socket error:", e)

    def handle_message(self, message):
        """
        Processes a message received from the thermostat.

        Message types:
            - TEMP:<current>,SETPOINT:<target>,HEAT:<1|0>
            - SCHEDULE:[hh:mm,temp],[hh:mm,temp]
            - ACK
        """

        message = message.strip()

        if message.startswith("TEMP:"):
            # Update GUI and internal variables based on thermostat state.
            # If an ACK is expected, confirm setpoint and mark as acknowledged.
            parts = message.split(",")
            temp = int(parts[0].split(":")[1])
            setpt = int(parts[1].split(":")[1])
            heat = bool(int(parts[2].split(":")[1]))

            self.temperature.set(temp)
            self.heat.set(heat)
            self.update_heat_indicator()

            # Always ACK the thermostat
            self.sock.sendall(b"ACK\n")

            # Only update GUI if on the main view
            if self.in_main_view:
                self.temperature.set(temp)
                self.heat.set(heat)
                self.update_heat_indicator()

                if self.waiting_for_ack:
                # Only clear ack if the thermostat confirms new setpoint
                    if setpt == self.pending_setpoint.get():
                        self.waiting_for_ack = False
                        self.setpoint.set(setpt)
                        self.setpoint_label.config(foreground="black")
                else:
                    # Normal case: update both setpoint and pending
                    self.setpoint.set(setpt)
                    self.pending_setpoint.set(setpt)
                    self.setpoint_label.config(foreground="black")
        elif message.startswith("SCHEDULE:["):
            try:
                import re
                matches = re.findall(r"\[(\d+):(\d+),(\d+)\]", message)
                if len(matches) == 2:
                    for i, (h, m, s) in enumerate(matches):
                        self.schedule[i]["hour"].set(int(h))
                        self.schedule[i]["minute"].set(int(m))
                        self.schedule[i]["setpoint"].set(int(s))
            except Exception as e:
                print("Error parsing schedule:", e)

        elif message == "ACK":
            pass



    def send_command(self, message):
        """
        Sends a string message to the thermostat over the socket.
        """
            
        try:
            if self.sock:
                self.sock.sendall((message + "\n").encode())
        except:
            pass

    def on_close(self):
        """
        Safely closes the socket and exits the GUI.
        """
        self.running = False
        if self.sock:
            self.sock.close()
        self.root.destroy()

if __name__ == "__main__":
    # Launch the GUI and register window close behavior
    root = tk.Tk()
    app = ThermostatGUI(root)
    root.protocol("WM_DELETE_WINDOW", app.on_close)
    root.mainloop()