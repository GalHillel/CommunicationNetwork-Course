import tkinter as tk
from tkinter import ttk
import subprocess


class Application(ttk.Frame):
    def __init__(self, master=None):
        super().__init__(master)
        self.master = master
        self.master.title("Application Launcher")
        self.master.geometry("500x300")
        self.style = ttk.Style()
        self.style.theme_use("clam")
        self.pack(fill=tk.BOTH, expand=True)
        self.create_widgets()

    def create_widgets(self):
        # Header
        header_label = ttk.Label(
            self, text="Application Launcher", font=("Helvetica", 24, "bold"))
        header_label.pack(side="top", pady=(10, 20))

        # HTTP Application Button
        http_button = ttk.Button(
            self, text="HTTP Application", command=self.activate_http)
        http_button.pack(pady=(0, 10), ipady=10, ipadx=30, padx=50, fill=tk.X)

        # DHCP and DNS Button
        dhcp_dns_button = ttk.Button(
            self, text="Activate The DHCP and DNS", command=self.activate_dhcp_dns)
        dhcp_dns_button.pack(pady=(0, 10), ipady=10,
                             ipadx=30, padx=50, fill=tk.X)

        # Client Button
        client_button = ttk.Button(
            self, text="Client", command=self.activate_client)
        client_button.pack(pady=(0, 10), ipady=10,
                           ipadx=30, padx=50, fill=tk.X)

    def activate_http(self):
        subprocess.Popen(["sudo", "python3", "./RUDPserver.py"])
        subprocess.Popen(["sudo", "python3", "./TCPserver.py"])
        subprocess.Popen(["sudo", "python3", "./Client.py"])

    def activate_dhcp_dns(self):
        subprocess.Popen(["sudo", "python3", "./DNS_Server.py"])
        subprocess.Popen(["sudo", "python3", "./DHCP_Server.py"])

    def activate_client(self):
        subprocess.Popen(["sudo", "python3", "./client1.py"])


root = tk.Tk()
app = Application(master=root)
app.mainloop()
