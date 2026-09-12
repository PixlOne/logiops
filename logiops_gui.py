import tkinter as tk
from tkinter import ttk, messagebox
import subprocess
import os
import glob
import re

KEY_OPTIONS = [
    "Super / Windows (KEY_LEFTMETA)",
    "Print Screen (KEY_SYSRQ)",
    "Copy (KEY_LEFTCTRL, KEY_C)",
    "Paste (KEY_LEFTCTRL, KEY_V)",
    "Cut (KEY_LEFTCTRL, KEY_X)",
    "Undo (KEY_LEFTCTRL, KEY_Z)",
    "Play/Pause (KEY_PLAYPAUSE)",
    "Next Song (KEY_NEXTSONG)",
    "Previous Song (KEY_PREVIOUSSONG)",
    "Mute (KEY_MUTE)",
    "Volume Up (KEY_VOLUMEUP)",
    "Volume Down (KEY_VOLUMEDOWN)",
    "Browser Back (KEY_BACK)",
    "Browser Forward (KEY_FORWARD)",
    "Show Desktop (KEY_LEFTMETA, KEY_D)",
    "Close Window (KEY_LEFTALT, KEY_F4)",
    "Terminal (KEY_LEFTCTRL, KEY_LEFTALT, KEY_T)",
    "Enter (KEY_ENTER)",
    "Space (KEY_SPACE)",
    "Escape (KEY_ESC)"
]

DPI_OPTIONS = [
    "400, 800, 1200, 1600",
    "800, 1200, 1600, 2000",
    "1000, 1500, 2000, 3000",
    "400, 600, 800, 1000, 1200, 1400, 1600"
]

class ButtonTab(ttk.Frame):
    def __init__(self, parent, button_name, cid):
        super().__init__(parent)
        self.button_name = button_name
        self.cid = cid
        
        self.enabled_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(self, text=f"Enable Custom Actions for {button_name} (CID: {cid})", variable=self.enabled_var, command=self.toggle_state).pack(pady=10, padx=10, anchor="w")
        
        self.gestures_frame = ttk.LabelFrame(self, text="Gestures & Actions")
        self.gestures_frame.pack(fill="both", expand=True, padx=10, pady=5)
        
        self.directions = [
            ("Single Press (No Swipe)", "None"),
            ("Swipe Up", "Up"),
            ("Swipe Down", "Down"),
            ("Swipe Left", "Left"),
            ("Swipe Right", "Right")
        ]
        
        self.action_vars = {}
        self.arg_vars = {}
        self.rows = []
        
        action_types = ["Do Nothing", "Keypress", "ToggleSmartshift", "CycleDPI"]
        
        for i, (label, direction) in enumerate(self.directions):
            ttk.Label(self.gestures_frame, text=f"{label}:").grid(row=i, column=0, sticky="w", padx=5, pady=5)
            
            act_var = tk.StringVar(value="Do Nothing")
            self.action_vars[direction] = act_var
            cb = ttk.Combobox(self.gestures_frame, textvariable=act_var, values=action_types, state="readonly", width=15)
            cb.grid(row=i, column=1, sticky="w", padx=5, pady=5)
            
            arg_var = tk.StringVar(value="")
            self.arg_vars[direction] = arg_var
            # Using Combobox for arguments so users can select from list OR type
            entry = ttk.Combobox(self.gestures_frame, textvariable=arg_var, width=32)
            entry.grid(row=i, column=2, sticky="w", padx=5, pady=5)
            
            hint_label = ttk.Label(self.gestures_frame, text="", font=("Arial", 8, "italic"))
            hint_label.grid(row=i, column=3, sticky="w", padx=5, pady=5)
            
            cb.bind("<<ComboboxSelected>>", lambda e, d=direction, entry=entry, hint=hint_label: self.on_action_change(d, entry, hint))
            
            self.rows.append((cb, entry, hint_label))
            
        self.toggle_state()
        
    def on_action_change(self, direction, entry, hint):
        act = self.action_vars[direction].get()
        if act == "Keypress":
            entry.configure(state="normal")
            entry['values'] = KEY_OPTIONS
            hint.configure(text="Select or type keycode(s)")
            if not self.arg_vars[direction].get():
                self.arg_vars[direction].set("Super / Windows (KEY_LEFTMETA)")
        elif act == "CycleDPI":
            entry.configure(state="normal")
            entry['values'] = DPI_OPTIONS
            hint.configure(text="Select or type DPI list")
            if not self.arg_vars[direction].get():
                self.arg_vars[direction].set("400, 800, 1200, 1600")
        else:
            entry.configure(state="disabled")
            entry['values'] = []
            hint.configure(text="")
            
    def toggle_state(self):
        state = "normal" if self.enabled_var.get() else "disabled"
        state_readonly = "readonly" if self.enabled_var.get() else "disabled"
        for cb, entry, hint in self.rows:
            cb.configure(state=state_readonly)
            act = cb.get()
            if act in ["Keypress", "CycleDPI"] and state == "normal":
                entry.configure(state="normal")
            else:
                entry.configure(state="disabled")

    def get_config(self):
        if not self.enabled_var.get():
            return ""
            
        gestures_cfg = []
        for label, direction in self.directions:
            act = self.action_vars[direction].get()
            arg = self.arg_vars[direction].get()
            
            if act == "Do Nothing":
                if direction == "None":
                    gestures_cfg.append(f'\n                    {{\n                        direction: "None";\n                        mode: "NoPress";\n                    }}')
                continue
                
            action_block = ""
            if act == "Keypress":
                # Find all KEY_ prefixed strings using regex
                keys = re.findall(r"KEY_[A-Z0-9_]+", arg)
                if not keys:
                    # Fallback if they didn't prefix (e.g. typing "A, B")
                    raw_keys = [k.strip() for k in arg.split(",") if k.strip()]
                    keys = []
                    for rk in raw_keys:
                        if not rk.startswith("KEY_"):
                            keys.append(f'"KEY_{rk.upper()}"')
                        else:
                            keys.append(f'"{rk}"')
                else:
                    keys = [f'"{k}"' for k in keys]
                keys_str = ", ".join(keys)
                action_block = f'type: "Keypress"; keys: [{keys_str}];'
            elif act == "ToggleSmartshift":
                action_block = 'type: "ToggleSmartshift";'
            elif act == "CycleDPI":
                dpis = re.findall(r"\d+", arg)
                dpis_str = ", ".join(dpis)
                action_block = f'type: "CycleDPI"; dpis: [{dpis_str}];'
                
            gestures_cfg.append(f"""
                    {{
                        direction: "{direction}";
                        mode: "OnRelease";
                        action = {{ {action_block} }};
                    }}""")
                    
        if not gestures_cfg:
            return ""
            
        gestures_str = ",".join(gestures_cfg)
        return f"""
        {{
            cid: {self.cid};
            action =
            {{
                type: "Gestures";
                gestures: ({gestures_str}
                );
            }};
        }}"""

class LogiOpsGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("LogiOps Customizer - Full Options")
        self.root.geometry("800x800")
        
        if os.geteuid() != 0:
            messagebox.showerror("Permission Error", "This application must be run as root (sudo).")
            self.root.destroy()
            return
            
        self.create_widgets()
        self.auto_detect_device()
        self.refresh_status()

    def create_widgets(self):
        style = ttk.Style()
        style.theme_use('clam')
        
        # --- Top: Device Name ---
        frame_device = ttk.Frame(self.root)
        frame_device.pack(fill="x", padx=10, pady=10)
        ttk.Label(frame_device, text="Device Name:", font=("Arial", 10, "bold")).pack(side="left", padx=5)
        self.device_name_var = tk.StringVar(value="Wireless Mouse MX Master 3")
        ttk.Entry(frame_device, textvariable=self.device_name_var, width=40).pack(side="left", padx=5)
        ttk.Button(frame_device, text="Auto-Detect", command=self.auto_detect_device).pack(side="left", padx=5)
        
        # --- Notebook for Tabs ---
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill="both", expand=True, padx=10, pady=5)
        
        # Tab 0: Status & Help
        self.tab_status = ttk.Frame(self.notebook)
        self.notebook.add(self.tab_status, text="Status & Help")
        self.setup_status_tab()
        
        # Tab 1: Pointer & Scroll
        self.tab_pointer = ttk.Frame(self.notebook)
        self.notebook.add(self.tab_pointer, text="Pointer & Scrolling")
        self.setup_pointer_tab()
        
        # Tab 2, 3, 4, 5: Buttons
        self.button_tabs = []
        buttons = [("Thumb Button", "0xc3"), ("Top Button", "0xc4"), ("Forward Button", "0x53"), ("Back Button", "0x56")]
        for name, cid in buttons:
            tab = ButtonTab(self.notebook, name, cid)
            self.notebook.add(tab, text=name)
            self.button_tabs.append(tab)
            
        # Optional: default thumb configuration setup
        self.button_tabs[0].enabled_var.set(True)
        self.button_tabs[0].toggle_state()
        self.button_tabs[0].action_vars["None"].set("Keypress")
        self.button_tabs[0].arg_vars["None"].set("Super / Windows (KEY_LEFTMETA)")
        self.button_tabs[0].on_action_change("None", self.button_tabs[0].rows[0][1], self.button_tabs[0].rows[0][2])

        # --- Actions ---
        frame_actions = ttk.Frame(self.root)
        frame_actions.pack(fill="x", padx=10, pady=15)
        ttk.Button(frame_actions, text="Save Configuration & Restart Logid", command=self.save_and_apply, width=40).pack(pady=5)

    def setup_status_tab(self):
        opts = {'padx': 10, 'pady': 5}
        
        frame_live = ttk.LabelFrame(self.tab_status, text="Live Status")
        frame_live.pack(fill="x", padx=10, pady=10)
        
        ttk.Label(frame_live, text="Service Status:").grid(row=0, column=0, sticky="w", **opts)
        self.lbl_svc_status = ttk.Label(frame_live, text="Checking...", font=("Arial", 10, "bold"))
        self.lbl_svc_status.grid(row=0, column=1, sticky="w", **opts)
        
        ttk.Label(frame_live, text="Device Connection:").grid(row=1, column=0, sticky="w", **opts)
        self.lbl_dev_status = ttk.Label(frame_live, text="Checking...", font=("Arial", 10, "bold"))
        self.lbl_dev_status.grid(row=1, column=1, sticky="w", **opts)
        
        ttk.Label(frame_live, text="Battery Status:").grid(row=2, column=0, sticky="w", **opts)
        self.lbl_bat_status = ttk.Label(frame_live, text="Checking...", font=("Arial", 10, "bold"))
        self.lbl_bat_status.grid(row=2, column=1, sticky="w", **opts)
        
        button_frame = ttk.Frame(frame_live)
        button_frame.grid(row=3, column=0, columnspan=2, pady=10)
        ttk.Button(button_frame, text="Refresh Status", command=self.refresh_status).pack(side="left", padx=5)
        ttk.Button(button_frame, text="Fix 'double run' Crash", command=self.fix_double_run).pack(side="left", padx=5)
        
        # Help Text
        frame_help = ttk.LabelFrame(self.tab_status, text="Instructions: Launching Custom Apps")
        frame_help.pack(fill="both", expand=True, padx=10, pady=10)
        
        help_text = """LogiOps works like a virtual keyboard and cannot directly execute scripts or launch apps.
To launch a custom application (like opening a terminal or Spotify) using a mouse button:

Step 1 (In this GUI):
Set the button action to "Keypress" and assign it a complex, unused shortcut.
Example: KEY_LEFTCTRL, KEY_LEFTALT, KEY_T (or KEY_LEFTMETA, KEY_X)

Step 2 (In Ubuntu Settings):
1. Open Ubuntu Settings -> Keyboard -> Keyboard Shortcuts -> Custom Shortcuts.
2. Add a new shortcut (Name: "My Custom App", Command: "my-app-command").
3. When prompted to record the shortcut, simply click your configured mouse button!

Your operating system will intercept the simulated keyboard shortcut from the mouse and launch the app!"""
        
        lbl_help = tk.Label(frame_help, text=help_text, justify="left", wraplength=750, font=("Arial", 10))
        lbl_help.pack(padx=10, pady=10, anchor="nw")

    def auto_detect_device(self):
        try:
            # Query system logs to see if a device was recently detected
            out = subprocess.check_output("journalctl -u logid -n 150", shell=True, text=True, stderr=subprocess.DEVNULL)
            matches = re.findall(r"Device found:\s*([^\n]+?)\s*on\s*/dev/", out)
            if matches:
                detected_name = matches[-1].strip()
                self.device_name_var.set(detected_name)
                return
        except:
            pass
            
        # Fallback to sysfs check
        paths = glob.glob("/sys/class/power_supply/*")
        for path in paths:
            try:
                with open(f"{path}/model_name", "r") as f:
                    model = f.read().strip()
                if "logi" in model.lower() or "mx" in model.lower():
                    self.device_name_var.set(model)
                    return
            except:
                pass

    def fix_double_run(self):
        try:
            # Stop service, kill all processes, and restart service
            subprocess.run(["systemctl", "stop", "logid"], stderr=subprocess.DEVNULL)
            subprocess.run(["killall", "logid"], stderr=subprocess.DEVNULL)
            subprocess.run(["systemctl", "start", "logid"], check=True)
            self.refresh_status()
            messagebox.showinfo("Success", "Terminated conflicting instances and successfully restarted the service!")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to fix: {str(e)}")

    def refresh_status(self):
        # Service status
        try:
            subprocess.check_output(["systemctl", "is-active", "logid"])
            self.lbl_svc_status.config(text="Active (Running)", foreground="green")
        except subprocess.CalledProcessError:
            self.lbl_svc_status.config(text="Inactive (Crashed or Stopped)", foreground="red")
            
        # Device Connection Status (Checking logiops dbus)
        dev_connected = False
        try:
            out = subprocess.check_output(["busctl", "tree", "pizza.pixl.LogiOps"], text=True, stderr=subprocess.DEVNULL)
            if "/pizza/pixl/LogiOps/Devices" in out:
                self.lbl_dev_status.config(text="Connected (Active)", foreground="green")
                dev_connected = True
            else:
                self.lbl_dev_status.config(text="Disconnected / Asleep", foreground="orange")
        except:
            self.lbl_dev_status.config(text="Unknown (Service not running)", foreground="gray")
            
        # Battery Status
        battery_found = False
        paths = glob.glob("/sys/class/power_supply/*")
        for path in paths:
            is_logi = False
            try:
                with open(f"{path}/model_name", "r") as f:
                    model = f.read().strip().lower()
                    if "logi" in model or "mx" in model: is_logi = True
            except: pass
            
            if "hidpp_battery" in path.lower() or "mac" in path.lower() or is_logi:
                try:
                    with open(f"{path}/capacity", "r") as f: cap = f.read().strip()
                    try:
                        with open(f"{path}/status", "r") as f: stat = f.read().strip()
                    except: stat = "Unknown"
                    self.lbl_bat_status.config(text=f"{cap}% ({stat})", foreground="blue")
                    battery_found = True
                    break
                except: pass
                
        if not battery_found:
            if dev_connected:
                self.lbl_bat_status.config(text="Not Reported by Kernel (Normal for logiops)", foreground="orange")
            else:
                self.lbl_bat_status.config(text="Not Found (Mouse Asleep or Unplugged)", foreground="gray")

    def setup_pointer_tab(self):
        opts = {'padx': 10, 'pady': 10}
        
        # DPI
        frame_dpi = ttk.LabelFrame(self.tab_pointer, text="DPI Setting")
        frame_dpi.pack(fill="x", padx=10, pady=5)
        self.dpi_var = tk.IntVar(value=1000)
        tk.Scale(frame_dpi, from_=200, to=4000, resolution=200, orient="horizontal", variable=self.dpi_var, length=300).pack(padx=10, pady=5)
        
        # SmartShift
        frame_smart = ttk.LabelFrame(self.tab_pointer, text="SmartShift (Freespin)")
        frame_smart.pack(fill="x", padx=10, pady=5)
        self.smartshift_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(frame_smart, text="Enable SmartShift", variable=self.smartshift_var).grid(row=0, column=0, columnspan=2, sticky="w", **opts)
        ttk.Label(frame_smart, text="Threshold (1-255):").grid(row=1, column=0, sticky="w", **opts)
        self.smart_threshold_var = tk.IntVar(value=30)
        tk.Scale(frame_smart, from_=1, to=255, orient="horizontal", variable=self.smart_threshold_var, length=200).grid(row=1, column=1, sticky="w", **opts)
        ttk.Label(frame_smart, text="Torque (1-255):").grid(row=2, column=0, sticky="w", **opts)
        self.smart_torque_var = tk.IntVar(value=50)
        tk.Scale(frame_smart, from_=1, to=255, orient="horizontal", variable=self.smart_torque_var, length=200).grid(row=2, column=1, sticky="w", **opts)
        
        # HiRes Scroll
        frame_hires = ttk.LabelFrame(self.tab_pointer, text="HiRes Scrolling")
        frame_hires.pack(fill="x", padx=10, pady=5)
        self.hiresscroll_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(frame_hires, text="Enable HiRes", variable=self.hiresscroll_var).grid(row=0, column=0, sticky="w", **opts)
        self.hires_invert_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(frame_hires, text="Invert Scroll", variable=self.hires_invert_var).grid(row=0, column=1, sticky="w", **opts)
        self.hires_target_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(frame_hires, text="Target", variable=self.hires_target_var).grid(row=0, column=2, sticky="w", **opts)

    def generate_config_string(self):
        name = self.device_name_var.get()
        dpi = self.dpi_var.get()
        
        smartshift = "true" if self.smartshift_var.get() else "false"
        threshold = self.smart_threshold_var.get()
        torque = self.smart_torque_var.get()
        
        hiresscroll = "true" if self.hiresscroll_var.get() else "false"
        invert = "true" if self.hires_invert_var.get() else "false"
        target = "true" if self.hires_target_var.get() else "false"
        
        buttons_configs = []
        for tab in self.button_tabs:
            cfg = tab.get_config()
            if cfg:
                buttons_configs.append(cfg)
                
        buttons_block = ""
        if buttons_configs:
            buttons_joined = ",".join(buttons_configs)
            buttons_block = f"\n    buttons: ({buttons_joined}\n    );"

        config = f"""devices: (
{{
    name: "{name}";
    smartshift:
    {{
        on: {smartshift};
        threshold: {threshold};
        torque: {torque};
    }};
    hiresscroll:
    {{
        hires: {hiresscroll};
        invert: {invert};
        target: {target};
    }};
    dpi: {dpi};{buttons_block}
}}
);
"""
        return config

    def save_and_apply(self):
        config_data = self.generate_config_string()
        try:
            with open("/etc/logid.cfg", "w") as f:
                f.write(config_data)
            subprocess.run(["systemctl", "restart", "logid"], check=True)
            self.refresh_status()
            messagebox.showinfo("Success", "Configuration saved and logid service restarted successfully!")
        except Exception as e:
            messagebox.showerror("Error", f"An error occurred: {str(e)}")

if __name__ == "__main__":
    root = tk.Tk()
    app = LogiOpsGUI(root)
    root.mainloop()
