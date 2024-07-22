import tkinter as tk
from tkinter import ttk
from tkinter import messagebox
from tkinter import scrolledtext
import serial
import serial.tools.list_ports

# Global variables
current_port = None
connection_status = False
ser = None

def search_ports():
    com_ports = [port.device for port in serial.tools.list_ports.comports()]
    port_combobox['values'] = com_ports
    if com_ports:
        port_combobox.current(0)  # Select the first COM port if available

def toggle_connection():
    global current_port, connection_status, ser

    selected_port = port_combobox.get()
    if selected_port and not connection_status:
        # Connect to the COM port
        try:
            ser = serial.Serial(selected_port, 115200, timeout=1)
            print(f"Connected to {selected_port}")
            current_port = selected_port
            connection_status = True
            connect_button.config(text="Disconnect")
            messagebox.showinfo("Connection Status", f"Connected to {selected_port}")

            # Display serial port parameters
            display_serial_params()

            # Start reading data from the serial port
            root.after(1000, read_from_port)

        except serial.SerialException as e:
            messagebox.showerror("Connection Error", f"Failed to connect to {selected_port}: {str(e)}")
    elif connection_status:
        # Disconnect from the COM port
        if ser:
            ser.close()
        print(f"Disconnected from {current_port}")
        current_port = None
        connection_status = False
        connect_button.config(text="Connect")
        messagebox.showinfo("Connection Status", f"Disconnected from {current_port}")

def display_serial_params():
    global port_params_text
    if ser:
        if 'port_params_text' in globals():
            # Clear previous content if exists
            port_params_text.delete(1.0, tk.END)
        else:
            port_params_text = scrolledtext.ScrolledText(root, wrap=tk.WORD, width=50, height=10)
            port_params_text.grid(row=10, column=1, columnspan=3, padx=10, pady=5, sticky='nsew')

        # Insert serial port parameters
        port_params_text.insert(tk.END, f"Port: {ser.port}\n")
        port_params_text.insert(tk.END, f"Baudrate: {ser.baudrate}\n")
        port_params_text.insert(tk.END, f"Timeout: {ser.timeout} seconds\n")

def read_from_port():
    global ser
    if ser is not None and ser.is_open:
        while ser.in_waiting > 0:
            try:
                data = ser.readline()
                if data:
                    print(f"Received data: {data.decode('utf-8').strip()}")
                    received_textbox.insert(tk.END, data.decode('utf-8'))
                    received_textbox.see(tk.END)  # Scroll to the end
            except serial.SerialException as e:
                print(f"Serial error: {str(e)}")
                break
            except Exception as e:
                print(f"Error: {str(e)}")

    root.after(10000, read_from_port)  # Continue reading

def get_data():
    if not connection_status:
        messagebox.showwarning("Connection Error", "Please connect to a valid COM port.")
        return
    
    try:
        # Read data from 'data.txt' file and fill the Entry fields
        with open('data.txt', 'r') as file:
            lines = file.readlines()
            for i, line in enumerate(lines):
                if i < len(text_boxes):
                    text_boxes[i].delete(0, tk.END)
                    text_boxes[i].insert(0, line.strip())

        messagebox.showinfo("Data Read", "Data loaded from file successfully.")
    except FileNotFoundError:
        messagebox.showerror("File Error", "File 'data.txt' not found.")
    except Exception as e:
        messagebox.showerror("Error", f"Failed to read data: {str(e)}")

def set_data():
    global ser

    if not connection_status:
        messagebox.showwarning("Connection Error", "Please connect to a valid COM port.")
        return
    
    station_value = station_var.get()
    wifi_name = text_boxes[0].get()
    password = text_boxes[1].get()
    url_firebase_1 = text_boxes[2].get()
    key_firebase_1 = text_boxes[3].get()
    url_firebase_2 = text_boxes[4].get()
    key_firebase_2 = text_boxes[5].get()

    # Dùng format string để tạo chuỗi dữ liệu với mã hex của 0xFF và 0xFE
    data_string = f"$$$*{station_value}*{wifi_name}*{password}*{url_firebase_1}*{key_firebase_1}*{url_firebase_2}*{key_firebase_2}*%%%"         
    print(f"Sending data: {data_string}")

    try:
        ser.write(data_string.encode())
    except serial.SerialException as e:
        messagebox.showerror("Serial Error", f"Failed to send data: {str(e)}")

def clear_received_data():
    received_textbox.delete(1.0, tk.END)

# Create the main window
root = tk.Tk()
root.title("IOT Setting Software")

# Set fixed window size and position
window_width = 800
window_height = 600
screen_width = root.winfo_screenwidth()
screen_height = root.winfo_screenheight()
x_coordinate = int((screen_width / 2) - (window_width / 2))
y_coordinate = int((screen_height / 2) - (window_height / 2))
root.geometry(f"{window_width}x{window_height}+{x_coordinate}+{y_coordinate}")

# Label "COM Port"
port_label = ttk.Label(root, text="COM Port:")
port_label.grid(row=1, column=0, padx=10, pady=5, sticky='e')

# Combobox to display COM port list
port_combobox = ttk.Combobox(root, width=27, state="readonly")
port_combobox.grid(row=1, column=1, padx=10, pady=5, columnspan=2)

# Search button
search_button = ttk.Button(root, text="Search", command=search_ports)
search_button.grid(row=1, column=3, padx=10, pady=5)

# Connect/Disconnect button
connect_button = ttk.Button(root, text="Connect", command=toggle_connection)
connect_button.grid(row=2, column=3, padx=10, pady=5)

# Get Data button
get_data_button = ttk.Button(root, text="Get Data", width=12, command=get_data)
get_data_button.grid(row=3, column=3, padx=10, pady=5)

# Set Data button
set_data_button = ttk.Button(root, text="Set Data", width=12, command=set_data)
set_data_button.grid(row=4, column=3, padx=10, pady=5)

# Clear Data button for received data
clear_received_button = ttk.Button(root, text="Clear", width=12, command=clear_received_data)
clear_received_button.grid(row=5, column=3, padx=10, pady=5)

# Number Box for station
station_label = ttk.Label(root, text="Station:")
station_label.grid(row=2, column=0, padx=10, pady=5, sticky='e')
station_var = tk.IntVar()
station_box = ttk.Spinbox(root, from_=1, to=20, textvariable=station_var, width=25)
station_box.grid(row=2, column=1, padx=10, pady=5, columnspan=2)

# Text Boxes for other parameters
labels = ["WiFi Name", "Password", "URL Firebase 1", "Key Firebase 1", "URL Firebase 2", "Key Firebase 2"]
text_boxes = []

for i, label_text in enumerate(labels):
    label = ttk.Label(root, text=label_text + ":")
    label.grid(row=i + 3, column=0, padx=10, pady=5, sticky='e')
    text_var = tk.StringVar()
    text_box = ttk.Entry(root, textvariable=text_var, width=25)
    text_box.grid(row=i + 3, column=1, padx=10, pady=5, columnspan=2)
    text_boxes.append(text_box)

# Label for received data
received_label = ttk.Label(root, text="Received Data:")
received_label.grid(row=9, column=0, padx=10, pady=5, sticky='e')

# ScrolledText for displaying received data with scrollbar
received_textbox = scrolledtext.ScrolledText(root, wrap=tk.WORD, width=70, height=10)
received_textbox.grid(row=9, column=1, columnspan=3, padx=10, pady=5, sticky='nsew')

# Start the main loop
root.mainloop()
