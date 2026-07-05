import tkinter as tk
from tkinter import messagebox
import serial
import serial.tools.list_ports
import time

# SmartBeaconing profiles — (low_spd, high_spd, slow_rate_min, fast_rate_sec, turn_min, turn_slope)
SB_PROFILES = {
    "OFF": None,
    "ON":  (10, 90, 10, 30, 28, 255),
}

SB_DESCRIPTIONS = {
    "OFF": "Fixed interval mode.\nBeacon is sent at every INTERVAL minutes regardless of speed.",
    "ON":  (
        "SmartBeaconing active. Interval field is ignored.\n"
        "\n"
        "  Speed              Beacon interval\n"
        "  ─────────────────────────────────\n"
        "   0 -  10 km/h  →  every 10 min\n"
        "  ~20 km/h        →  every  8.5 min\n"
        "  ~30 km/h        →  every  7 min\n"
        "  ~50 km/h        →  every  5 min\n"
        "  ~70 km/h        →  every  2.5 min\n"
        "  ~80 km/h        →  every  1.5 min\n"
        "   90+ km/h       →  every 30 sec"
    ),
}

class BlindTRackConfigurator:
    def __init__(self, root):
        self.root = root
        self.root.title("BlindTRack Configurator Terminal")
        self.root.geometry("500x600")
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

        self.sb_var = tk.StringVar(value="OFF")
        self.create_widgets()
        self.refresh_ports()

    # ── Validation ──────────────────────────────────────────────────

    def validate_callsign(self, P):
        if len(P) > 9: return False
        return all(c.isalnum() for c in P) if P else True

    def validate_ssid(self, P):
        if P == "": return True
        if not P.isdigit(): return False
        return 0 <= int(P) <= 15

    def validate_interval(self, P):
        if P == "": return True
        if not P.isdigit(): return False
        if P.startswith("0"): return False
        return int(P) <= 99

    def validate_comment(self, P):
        return len(P) <= 49

    # ── Widgets ────────────────────────────────────────────────────

    def create_widgets(self):
        f      = ("Consolas", 11)
        f_desc = ("Consolas", 9)
        bg, fg = "#0a0a0a", "#00ff41"
        btn_bg   = "#1a1a1a"
        btn_blue = "#3399ff"
        dim_fg   = "#444444"

        # Title
        title_frame = tk.Frame(self.root, bg=bg)
        title_frame.pack(pady=12)
        tk.Label(title_frame, text="[",             bg=bg, fg=fg,        font=("Consolas", 14, "bold")).pack(side="left")
        tk.Label(title_frame, text="BlindTRack ",   bg=bg, fg="#3399ff", font=("Consolas", 14, "bold")).pack(side="left")
        tk.Label(title_frame, text="APRS TRACKER]", bg=bg, fg=fg,        font=("Consolas", 14, "bold")).pack(side="left")

        frame = tk.Frame(self.root, bg=bg)
        frame.pack(fill="both", expand=True, padx=20)
        frame.columnconfigure(1, weight=1)

        row = 0

        # Port
        tk.Label(frame, text="COM PORT:", bg=bg, fg=fg, font=f).grid(row=row, column=0, sticky="w", pady=4)
        self.port_var = tk.StringVar()
        self.port_menu = tk.OptionMenu(frame, self.port_var, "")
        self.port_menu.config(bg="#121212", fg=fg, font=f)
        self.port_menu.grid(row=row, column=1, sticky="ew", padx=(0,5))
        tk.Button(frame, text="REFRESH", command=self.refresh_ports,
                  bg=btn_bg, fg=fg, font=f, relief="flat",
                  activebackground=btn_bg, activeforeground=btn_blue
                  ).grid(row=row, column=2, sticky="ew", padx=5)
        row += 1

        # Standard fields
        self.entries = {}
        fields = [
            ("CALLSIGN:",       "NOCALL", self.vcmd_callsign, "max 9"),
            ("SSID (0-15):",    "12",     self.vcmd_ssid,     "0-15"),
            ("INTERVAL (Min):", "2",      self.vcmd_interval, "1-99"),
        ]
        for lbl, val, vcmd, hint in fields:
            tk.Label(frame, text=lbl, bg=bg, fg=fg, font=f).grid(row=row, column=0, sticky="w", pady=4)
            e = tk.Entry(frame, bg="#121212", fg=fg, font=f, insertbackground=fg,
                         validate="key", validatecommand=vcmd)
            e.insert(0, val)
            e.grid(row=row, column=1, sticky="ew", pady=4)
            tk.Label(frame, text=hint, bg=bg, fg=dim_fg, font=f_desc).grid(row=row, column=2, sticky="w", padx=4)
            self.entries[lbl] = e
            row += 1

        # Symbol
        tk.Label(frame, text="SYMBOL:", bg=bg, fg=fg, font=f).grid(row=row, column=0, sticky="w", pady=4)
        self.sym_var = tk.StringVar(value=list(self.symbols.keys())[0])
        self.sym_menu = tk.OptionMenu(frame, self.sym_var, *self.symbols.keys())
        self.sym_menu.config(bg="#121212", fg=fg, font=f)
        self.sym_menu.grid(row=row, column=1, columnspan=2, sticky="ew", pady=4)
        row += 1

        # Comment
        tk.Label(frame, text="COMMENT:", bg=bg, fg=fg, font=f).grid(row=row, column=0, sticky="w", pady=4)
        self.comment_e = tk.Entry(frame, bg="#121212", fg=fg, font=f, insertbackground=fg,
                                  validate="key", validatecommand=self.vcmd_comment)
        self.comment_e.insert(0, "BlindTRack STM32 Tracker")
        self.comment_e.grid(row=row, column=1, sticky="ew", pady=4)
        tk.Label(frame, text="max 49", bg=bg, fg=dim_fg, font=f_desc).grid(row=row, column=2, sticky="w", padx=4)
        row += 1

        # ── SmartBeaconing separator ──
        tk.Frame(frame, bg="#1a3a1a", height=1).grid(row=row, column=0, columnspan=3, sticky="ew", pady=(10,6))
        row += 1

        # SmartBeaconing ON/OFF
        sb_hdr = tk.Frame(frame, bg=bg)
        sb_hdr.grid(row=row, column=0, columnspan=3, sticky="ew", pady=(0,4))
        tk.Label(sb_hdr, text="SMART BEACON:", bg=bg, fg=fg, font=f).pack(side="left")
        for val in ("OFF", "ON"):
            tk.Radiobutton(sb_hdr, text=val, variable=self.sb_var, value=val,
                           bg=bg, fg=fg, selectcolor="#1a1a1a",
                           activebackground=bg, activeforeground="#3399ff",
                           font=f, command=self.on_sb_toggle).pack(side="left", padx=(12,0))
        row += 1

        # Description box
        self.sb_desc = tk.Label(frame, text=SB_DESCRIPTIONS["OFF"],
                                bg="#0d1a0d", fg="#4a8a4a",
                                font=("Consolas", 9), justify="left",
                                anchor="w", padx=8, pady=6,
                                wraplength=0)
        self.sb_desc.grid(row=row, column=0, columnspan=3, sticky="ew", pady=(0,10))
        row += 1

        # Buttons
        btn_f = tk.Frame(frame, bg=bg)
        btn_f.grid(row=row, column=0, columnspan=3, pady=(4,0), sticky="ew")
        tk.Button(btn_f, text="READ FROM DEVICE", command=self.read_cfg,
                  bg=btn_bg, fg=fg, font=f,
                  activebackground=btn_bg, activeforeground=btn_blue
                  ).pack(side="left", expand=True, fill="x", padx=(0,5))
        tk.Button(btn_f, text="WRITE TO DEVICE", command=self.write_cfg,
                  bg=btn_bg, fg=fg, font=f,
                  activebackground=btn_bg, activeforeground=btn_blue
                  ).pack(side="right", expand=True, fill="x", padx=(5,0))

        # Footer
        footer_frame = tk.Frame(self.root, bg=bg)
        footer_frame.pack(side="bottom", fill="x", pady=8)
        footer_label = tk.Label(footer_frame, text="ercanolcay.com", bg=bg, fg=fg,
                                font=("Consolas", 10), cursor="hand2")
        footer_label.pack()
        footer_label.bind("<Button-1>", lambda e: self.open_website())

    # ── SmartBeaconing toggle ───────────────────────────────────────

    def on_sb_toggle(self):
        sb_on = self.sb_var.get() == "ON"
        self.sb_desc.config(text=SB_DESCRIPTIONS["ON" if sb_on else "OFF"])
        interval_e = self.entries["INTERVAL (Min):"]
        if sb_on:
            interval_e.config(state="disabled", fg="#2a5a2a")
        else:
            interval_e.config(state="normal", fg="#00ff41")

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

    # ── Final validation ────────────────────────────────────────────

    def _validate_all(self):
        callsign = self.entries["CALLSIGN:"].get().strip().upper()
        if not callsign or len(callsign) > 9:
            return "Callsign must be 1-9 characters!"
        if not all(c.isalnum() for c in callsign):
            return "Callsign: letters and digits only!"

        ssid_str = self.entries["SSID (0-15):"].get()
        if not ssid_str.isdigit() or not (0 <= int(ssid_str) <= 15):
            return "SSID must be 0-15!"

        if self.sb_var.get() == "OFF":
            interval_str = self.entries["INTERVAL (Min):"].get()
            if not interval_str.isdigit() or not (1 <= int(interval_str) <= 99):
                return "Interval must be 1-99 minutes!"

        if len(self.comment_e.get()) > 49:
            return "Comment max 49 characters!"

        return None

    # ── Read / Write ───────────────────────────────────────────────

    def _disable_validation(self):
        for e in self.entries.values():
            e.config(validate="none")
        self.comment_e.config(validate="none")

    def _enable_validation(self):
        for e in self.entries.values():
            e.config(validate="key")
        self.comment_e.config(validate="key")

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
                    p = res[4:].split(',', 11)
                    if len(p) < 5:
                        messagebox.showerror("Error", "Incomplete response from device!")
                        return

                    self._disable_validation()

                    self.entries["CALLSIGN:"].delete(0, tk.END)
                    self.entries["CALLSIGN:"].insert(0, p[0][:9])
                    self.entries["SSID (0-15):"].delete(0, tk.END)
                    self.entries["SSID (0-15):"].insert(0, p[1])
                    self.entries["INTERVAL (Min):"].delete(0, tk.END)
                    self.entries["INTERVAL (Min):"].insert(0, p[2])
                    for k, v in self.symbols.items():
                        if v == p[3]: self.sym_var.set(k)

                    if len(p) >= 12:
                        sb_on = p[4].isdigit() and int(p[4]) == 1
                        self.sb_var.set("ON" if sb_on else "OFF")
                        self.on_sb_toggle()
                        self.comment_e.delete(0, tk.END)
                        self.comment_e.insert(0, p[11][:49])
                    else:
                        self.sb_var.set("OFF")
                        self.on_sb_toggle()
                        self.comment_e.delete(0, tk.END)
                        self.comment_e.insert(0, p[4][:49] if len(p) > 4 else "")

                    self._enable_validation()
                    messagebox.showinfo("Success", "Configuration loaded from BlindTRack!")
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

        if self.sb_var.get() == "ON":
            vals = SB_PROFILES["ON"]
            sb, low, high, slow, fast, turn_min, turn_sl = 1, *vals
        else:
            sb, low, high, slow, fast, turn_min, turn_sl = 0, 10, 90, 10, 30, 28, 255

        payload = f"CFG:{callsign},{ssid},{interval},{sym},{sb},{low},{high},{slow},{fast},{turn_min},{turn_sl},{comment}\n"
        try:
            with serial.Serial(port, 9600, timeout=1) as ser:
                ser.write(payload.encode('ascii'))
            messagebox.showinfo("Success", "Configuration saved to BlindTRack!")
        except Exception as e:
            messagebox.showerror("Error", str(e))

if __name__ == "__main__":
    root = tk.Tk()
    BlindTRackConfigurator(root)
    root.mainloop()
