import tkinter as tk
from tkinter import messagebox
import serial
import serial.tools.list_ports
import time

class BlueTRaceConfigurator:
    def __init__(self, root):
        self.root = root
        self.root.title("BlueTRace Configurator Terminal")
        self.root.geometry("500x370")
        self.root.configure(bg="#0a0a0a")

        self.symbols = {
            "/( (Satellite Car)": "/(",
            "/> (Standard Car)": "/>",
            "/v (Van/Comm Vehicle)": "/v",
            "/u (Bus/Truck)": "/u",
            "/[ (Runner/Human)": "/[",
            "/- (Home Station)": "/-",
            "/O (Balloon)": "/O",
            "/Y (Yacht/Boat)": "/Y"
        }

        self.vcmd_interval = (self.root.register(self.validate_interval), '%P')
        self.vcmd_callsign = (self.root.register(self.validate_callsign), '%P')
        self.vcmd_ssid     = (self.root.register(self.validate_ssid),     '%P')
        self.vcmd_comment  = (self.root.register(self.validate_comment),  '%P')

        self.create_widgets()
        self.refresh_ports()

    # ── Validation ─────────────────────────────────────────────────

    def validate_callsign(self, P):
        """Max 9 karakter, sadece harf ve rakam (APRS standardı)"""
        if len(P) > 9:
            return False
        return all(c.isalnum() for c in P) if P else True

    def validate_ssid(self, P):
        """0-15 arası integer"""
        if P == "":
            return True
        if not P.isdigit():
            return False
        return 0 <= int(P) <= 15

    def validate_interval(self, P):
        """1-99 arası, 0 ile başlamasın"""
        if P == "":
            return True
        if not P.isdigit():
            return False
        if P.startswith("0"):
            return False
        return int(P) <= 99

    def validate_comment(self, P):
        """Max 49 karakter"""
        return len(P) <= 49

    # ── Widgets ─────────────────────────────────────────────────────

    def create_widgets(self):
        f = ("Consolas", 11)
        bg, fg = "#0a0a0a", "#00ff41"
        btn_bg = "#1a1a1a"
        btn_hover_text_blue = "#3399ff"

        title_frame = tk.Frame(self.root, bg=bg)
        title_frame.pack(pady=15)
        tk.Label(title_frame, text="[",            bg=bg, fg=fg,            font=("Consolas", 14, "bold")).pack(side="left")
        tk.Label(title_frame, text="BlueTRace ",   bg=bg, fg="#3399ff",     font=("Consolas", 14, "bold")).pack(side="left")
        tk.Label(title_frame, text="APRS TRACKER]",bg=bg, fg=fg,            font=("Consolas", 14, "bold")).pack(side="left")

        frame = tk.Frame(self.root, bg=bg)
        frame.pack(fill="both", expand=True, padx=20)

        # Port
        tk.Label(frame, text="COM PORT:", bg=bg, fg=fg, font=f).grid(row=0, column=0, sticky="w", pady=5)
        self.port_var = tk.StringVar()
        self.port_menu = tk.OptionMenu(frame, self.port_var, "")
        self.port_menu.config(bg="#121212", fg=fg, font=f)
        self.port_menu.grid(row=0, column=1, sticky="ew", padx=(0, 5))
        tk.Button(frame, text="REFRESH", command=self.refresh_ports,
                  bg=btn_bg, fg=fg, font=f, relief="flat",
                  activebackground=btn_bg, activeforeground=btn_hover_text_blue
                  ).grid(row=0, column=2, sticky="ew", padx=5)

        # Input fields — validation + max length hint label
        self.entries = {}
        fields = [
            ("CALLSIGN:",      "NOCALL",  self.vcmd_callsign, "max 9"),
            ("SSID (0-15):",   "12",      self.vcmd_ssid,     "0-15"),
            ("INTERVAL (Min):","2",       self.vcmd_interval, "1-99"),
        ]
        for i, (lbl, val, vcmd, hint) in enumerate(fields):
            tk.Label(frame, text=lbl, bg=bg, fg=fg, font=f).grid(row=i+1, column=0, sticky="w", pady=5)
            e = tk.Entry(frame, bg="#121212", fg=fg, font=f, insertbackground=fg,
                         validate="key", validatecommand=vcmd)
            e.insert(0, val)
            e.grid(row=i+1, column=1, sticky="ew", pady=5)
            tk.Label(frame, text=hint, bg=bg, fg="#444444", font=("Consolas", 9)).grid(row=i+1, column=2, sticky="w", padx=4)
            self.entries[lbl] = e

        # Symbol selector
        tk.Label(frame, text="SYMBOL:", bg=bg, fg=fg, font=f).grid(row=4, column=0, sticky="w", pady=5)
        self.sym_var = tk.StringVar(value=list(self.symbols.keys())[0])
        self.sym_menu = tk.OptionMenu(frame, self.sym_var, *self.symbols.keys())
        self.sym_menu.config(bg="#121212", fg=fg, font=f)
        self.sym_menu.grid(row=4, column=1, columnspan=2, sticky="ew", pady=5)

        # Comment field
        tk.Label(frame, text="COMMENT:", bg=bg, fg=fg, font=f).grid(row=5, column=0, sticky="w", pady=5)
        self.comment_e = tk.Entry(frame, bg="#121212", fg=fg, font=f, insertbackground=fg,
                                  validate="key", validatecommand=self.vcmd_comment)
        self.comment_e.insert(0, "BlueTRace STM32 Tracker")
        self.comment_e.grid(row=5, column=1, sticky="ew", pady=5)
        tk.Label(frame, text="max 49", bg=bg, fg="#444444", font=("Consolas", 9)).grid(row=5, column=2, sticky="w", padx=4)

        # Buttons
        btn_f = tk.Frame(frame, bg=bg)
        btn_f.grid(row=6, column=0, columnspan=3, pady=(10, 0), sticky="ew")
        tk.Button(btn_f, text="READ FROM DEVICE", command=self.read_cfg,
                  bg=btn_bg, fg=fg, font=f,
                  activebackground=btn_bg, activeforeground=btn_hover_text_blue
                  ).pack(side="left", expand=True, fill="x", padx=(0, 5))
        tk.Button(btn_f, text="WRITE TO DEVICE", command=self.write_cfg,
                  bg=btn_bg, fg=fg, font=f,
                  activebackground=btn_bg, activeforeground=btn_hover_text_blue
                  ).pack(side="right", expand=True, fill="x", padx=(5, 0))

        # Footer
        footer_frame = tk.Frame(self.root, bg=bg)
        footer_frame.pack(side="bottom", fill="x", pady=10)
        footer_label = tk.Label(footer_frame, text="ercanolcay.com", bg=bg, fg=fg,
                                font=("Consolas", 10), cursor="hand2")
        footer_label.pack()
        footer_label.bind("<Button-1>", lambda e: self.open_website())

    def open_website(self):
        import webbrowser
        webbrowser.open_new("http://ercanolcay.com")

    def refresh_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        menu = self.port_menu["menu"]
        menu.delete(0, "end")
        for p in ports:
            menu.add_command(label=p, command=lambda v=p: self.port_var.set(v))
        self.port_var.set(ports[0] if ports else "N/A")

    # ── Final validation before write ──────────────────────────────

    def _validate_all(self):
        """Tüm alanları yazma öncesi son kez kontrol eder, hata mesajı döner."""
        callsign = self.entries["CALLSIGN:"].get().strip().upper()
        if not callsign or len(callsign) > 9:
            return "Callsign 1-9 karakter olmalı!"
        if not all(c.isalnum() for c in callsign):
            return "Callsign sadece harf ve rakam içerebilir!"

        ssid_str = self.entries["SSID (0-15):"].get()
        if not ssid_str.isdigit() or not (0 <= int(ssid_str) <= 15):
            return "SSID 0-15 arasında olmalı!"

        interval_str = self.entries["INTERVAL (Min):"].get()
        if not interval_str.isdigit() or not (1 <= int(interval_str) <= 99):
            return "Interval 1-99 dakika arasında olmalı!"

        comment = self.comment_e.get()
        if len(comment) > 49:
            return "Comment maksimum 49 karakter olabilir!"

        return None  # No errors

    # ── Read / Write ────────────────────────────────────────────────

    def read_cfg(self):
        port = self.port_var.get()
        if port in ("N/A", ""):
            return
        try:
            with serial.Serial(port, 9600, timeout=2) as ser:
                ser.flushInput()
                ser.write(b"GET_CFG\n")
                time.sleep(0.5)
                res = ser.readline().decode('ascii', errors='ignore').strip()
                if res.startswith("CFG:"):
                    p = res[4:].split(',', 4)
                    if len(p) < 5:
                        messagebox.showerror("Error", "Incomplete response from device!")
                        return
                    # Temporarily disable validation to allow programmatic insert
                    for e in self.entries.values():
                        e.config(validate="none")
                    self.comment_e.config(validate="none")

                    self.entries["CALLSIGN:"].delete(0, tk.END);       self.entries["CALLSIGN:"].insert(0, p[0][:9])
                    self.entries["SSID (0-15):"].delete(0, tk.END);    self.entries["SSID (0-15):"].insert(0, p[1])
                    self.entries["INTERVAL (Min):"].delete(0, tk.END); self.entries["INTERVAL (Min):"].insert(0, p[2])
                    self.comment_e.delete(0, tk.END);                  self.comment_e.insert(0, p[4][:49])
                    for k, v in self.symbols.items():
                        if v == p[3]:
                            self.sym_var.set(k)

                    # Re-enable validation
                    for e in self.entries.values():
                        e.config(validate="key")
                    self.comment_e.config(validate="key")

                    messagebox.showinfo("Success", "Configuration loaded from BlueTRace!")
                else:
                    messagebox.showerror("Error", "No valid response. Check GND or Port!")
        except Exception as e:
            messagebox.showerror("Error", str(e))

    def write_cfg(self):
        port = self.port_var.get()
        if port in ("N/A", ""):
            return

        err = self._validate_all()
        if err:
            messagebox.showwarning("Warning", err)
            return

        callsign = self.entries["CALLSIGN:"].get().strip().upper()
        ssid     = self.entries["SSID (0-15):"].get()
        interval = self.entries["INTERVAL (Min):"].get()
        sym      = self.symbols[self.sym_var.get()]
        comment  = self.comment_e.get()

        payload = f"CFG:{callsign},{ssid},{interval},{sym},{comment}\n"
        try:
            with serial.Serial(port, 9600, timeout=1) as ser:
                ser.write(payload.encode('ascii'))
            messagebox.showinfo("Success", "Configuration saved to BlueTRace!")
        except Exception as e:
            messagebox.showerror("Error", str(e))

if __name__ == "__main__":
    root = tk.Tk()
    BlueTRaceConfigurator(root)
    root.mainloop()
