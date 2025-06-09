import tkinter as tk
from tkinter import ttk, messagebox
import threading
import firebase_admin
from firebase_admin import credentials, db
import pandas as pd

#Firebase Setup
path = r"path to key"
url = r"url to database"
cred = credentials.Certificate(path)

firebase_admin.initialize_app(cred, {
    'databaseURL': url
})

#
root = tk.Tk()
root.title("Room Occupancy Monitor")

# People count display
count_var = tk.StringVar(value="Loading...")
count_label = ttk.Label(root, text="People Count:", font=("Arial", 14))

count_label.pack(pady=10)

count_value = ttk.Label(root, textvariable=count_var, font=("Arial", 20, "bold"))

count_value.pack(pady=5)


log_label = ttk.Label(root, text="Recent Logs:", font=("Arial", 14))
log_label.pack(pady=10)

log_box = tk.Text(root, height=10, width=50)
log_box.pack(pady=5)

def update_gui(snapshot):
    # Update people count
    people = db.reference("room/people_count").get()
    count_var.set(str(people))

    # Fetch logs
    logs = db.reference("room/logs").get()
    if logs:
        log_box.delete("1.0", tk.END)
        # Sort by timestamp descending
        sorted_logs = sorted(logs.values(), key=lambda x: x.get('time', 0), reverse=True)
        for log in sorted_logs:  # Show last 10
            log_box.insert(tk.END, f"{log.get('time')} - {log.get('action')} (count: {log.get('count')})\n")
            log_box.yview_moveto(0)


def listener(event):
    update_gui(event)

def firebase_listener():
    db.reference("room").listen(listener)

def export_logs_to_csv():
    logs = db.reference("room/logs").get()
    if logs:
        df = pd.DataFrame(logs.values())
        df.to_csv("logs.csv", index=False)
    messagebox.showinfo("Export Complete", "Logs have been saved as logs.csv")

export_button = ttk.Button(root, text="Export Logs to CSV", command=export_logs_to_csv)
export_button.pack(pady=10)

# Start Firebase listener in a thread to keep UI responsive
threading.Thread(target=firebase_listener, daemon=True).start()

root.mainloop()
