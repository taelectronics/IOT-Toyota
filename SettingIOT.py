import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import serial
import serial.tools.list_ports
import threading

class SerialApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Serial Communication")
        self.root.geometry("600x500")  # Tăng kích thước cửa sổ

        self.serial_connection = None

        # Combobox for COM ports
        self.com_label = tk.Label(root, text="COM Port")
        self.com_label.grid(row=0, column=0)
        self.combobox = ttk.Combobox(root, state="readonly")
        self.combobox.grid(row=0, column=1, padx=5, pady=5)

        # Numberbox for Station
        self.station_label = tk.Label(root, text="Station")
        self.station_label.grid(row=1, column=0)
        self.station_var = tk.IntVar(value=1)
        self.station_entry = tk.Entry(root, textvariable=self.station_var)
        self.station_entry.grid(row=1, column=1, padx=5, pady=5)

        # Textbox for SSID
        self.ssid_label = tk.Label(root, text="SSID")
        self.ssid_label.grid(row=2, column=0)
        self.ssid_entry = tk.Entry(root)
        self.ssid_entry.grid(row=2, column=1, padx=5, pady=5)

        # Textbox for Password
        self.password_label = tk.Label(root, text="Password")
        self.password_label.grid(row=3, column=0)
        self.password_entry = tk.Entry(root, show="*")
        self.password_entry.grid(row=3, column=1, padx=5, pady=5)

        # Buttons
        self.search_button = tk.Button(root, text="Search", command=self.search_com_ports)
        self.search_button.grid(row=4, column=0, padx=5, pady=5)

        self.connect_button = tk.Button(root, text="Connect", command=self.connect_com_port)
        self.connect_button.grid(row=4, column=1, padx=5, pady=5)

        self.disconnect_button = tk.Button(root, text="Disconnect", command=self.disconnect_com_port)
        self.disconnect_button.grid(row=4, column=2, padx=5, pady=5)
        self.disconnect_button.config(state=tk.DISABLED)

        self.getdata_button = tk.Button(root, text="Get Data", command=self.get_data)
        self.getdata_button.grid(row=5, column=0, padx=5, pady=5)

        self.setdata_button = tk.Button(root, text="Set Data", command=self.set_data)
        self.setdata_button.grid(row=5, column=1, padx=5, pady=5)

        # Textbox to display sent packets
        self.sent_packets_label = tk.Label(root, text="Sent Packets")
        self.sent_packets_label.grid(row=6, column=0, padx=5, pady=5)
        self.sent_packets_text = tk.Text(root, height=10, width=50)
        self.sent_packets_text.grid(row=6, column=1, columnspan=2, padx=5, pady=5)

        # Textbox to display received data
        self.received_data_label = tk.Label(root, text="Received Data")
        self.received_data_label.grid(row=7, column=0, padx=5, pady=5)
        self.received_data_text = tk.Text(root, height=10, width=50)
        self.received_data_text.grid(row=7, column=1, columnspan=2, padx=5, pady=5)

        # Start a thread to read data from the serial port
        self.reading_thread = threading.Thread(target=self.read_from_serial)
        self.reading_thread.daemon = True
        self.reading_thread.start()

    def search_com_ports(self):
        ports = serial.tools.list_ports.comports()
        port_list = [port.device for port in ports]
        self.combobox['values'] = port_list
        if port_list:
            self.combobox.current(0)
        else:
            messagebox.showinfo("Info", "No COM ports found")

    def connect_com_port(self):
        try:
            com_port = self.combobox.get()
            if com_port:
                self.serial_connection = serial.Serial(com_port, 115200, timeout=1)
                self.connect_button.config(state=tk.DISABLED)
                self.disconnect_button.config(state=tk.NORMAL)
                messagebox.showinfo("Info", f"Connected to {com_port}")
            else:
                messagebox.showwarning("Warning", "Select a COM port first")
        except Exception as e:
            messagebox.showerror("Error", str(e))

    def disconnect_com_port(self):
        try:
            if self.serial_connection and self.serial_connection.is_open:
                self.serial_connection.close()
                self.connect_button.config(state=tk.NORMAL)
                self.disconnect_button.config(state=tk.DISABLED)
                messagebox.showinfo("Info", "Disconnected")
            else:
                messagebox.showwarning("Warning", "Not connected to any COM port")
        except Exception as e:
            messagebox.showerror("Error", str(e))

    def get_data(self):
        try:
            file_path = filedialog.askopenfilename(filetypes=[("Text files", "*.txt")])
            if file_path:
                with open(file_path, 'r') as file:
                    lines = file.readlines()
                    if len(lines) >= 3:
                        self.station_var.set(int(lines[0].strip()))
                        self.ssid_entry.delete(0, tk.END)
                        self.ssid_entry.insert(0, lines[1].strip())
                        self.password_entry.delete(0, tk.END)
                        self.password_entry.insert(0, lines[2].strip())
                    else:
                        messagebox.showwarning("Warning", "File does not have enough lines")
        except Exception as e:
            messagebox.showerror("Error", str(e))

    def set_data(self):
        try:
            if self.serial_connection and self.serial_connection.is_open:
                station = self.station_var.get()
                ssid = self.ssid_entry.get()
                password = self.password_entry.get()
                message = f"$$$*{station}*{ssid}*{password}*%%%"
                self.serial_connection.write(message.encode())
                self.sent_packets_text.insert(tk.END, f"Sent: {message}\n")
                messagebox.showinfo("Info", "Data sent")
            else:
                messagebox.showwarning("Warning", "Not connected to any COM port")
        except Exception as e:
            messagebox.showerror("Error", str(e))

    def read_from_serial(self):
        while True:
            if self.serial_connection and self.serial_connection.is_open:
                try:
                    data = self.serial_connection.readline().decode().strip()
                    if data:
                        self.received_data_text.insert(tk.END, f"Received: {data}\n")
                        self.received_data_text.see(tk.END)
                except Exception as e:
                    messagebox.showerror("Error", str(e))

if __name__ == "__main__":
    root = tk.Tk()
    app = SerialApp(root)
    root.mainloop()
