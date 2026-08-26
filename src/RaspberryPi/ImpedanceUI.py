#!/usr/bin/env python3

import tkinter as tk
from multiprocessing import shared_memory


# ---------------------------------------------------------
# Shared memory
# ---------------------------------------------------------

IMPEDANCE_SHM_NAME = "dsi_impedance"
CONTROL_SHM_NAME = "dsi_impedance_ctrl"

IMPEDANCE_SHM_SIZE = 256


# ---------------------------------------------------------
# Impedance thresholds, in Ohms
# ---------------------------------------------------------

GOOD_THRESHOLD = 1_000_000       # < 1 MΩ
CHECK_THRESHOLD = 2_000_000      # 1–2 MΩ
                                  # > 2 MΩ = poor


# ---------------------------------------------------------
# Electrode positions
#
# For now these are the six electrodes you're currently
# using. We can make this dynamic later.
# ---------------------------------------------------------

ELECTRODES = {
    "F3": (170, 150),
    "F4": (330, 150),

    "C3": (150, 260),
    "C4": (350, 260),

    "S1": (180, 370),
    "S3": (320, 370),
}


class ImpedanceUI:

    def __init__(self, root):

        self.root = root

        self.root.title("NeuroMove - Headset Setup")
        self.root.geometry("520x650")

        self.impedance_memory = None
        self.control_memory = None

        self.electrode_circles = {}
        self.electrode_values = {}

        # -------------------------------------------------
        # Main canvas
        # -------------------------------------------------

        self.canvas = tk.Canvas(
            root,
            width=500,
            height=510,
            bg="white"
        )

        self.canvas.pack(pady=10)

        self.draw_head()
        self.draw_electrodes()
        self.draw_legend()

        # -------------------------------------------------
        # Status text
        # -------------------------------------------------

        self.status_label = tk.Label(
            root,
            text="Waiting for TinyBCI...",
            font=("Arial", 14)
        )

        self.status_label.pack(pady=5)

        # -------------------------------------------------
        # Start session button
        # -------------------------------------------------

        self.start_button = tk.Button(
            root,
            text="Start Session",
            font=("Arial", 16, "bold"),
            command=self.start_session,
            state=tk.DISABLED
        )

        self.start_button.pack(
            ipadx=20,
            ipady=8,
            pady=10
        )

        # Try connecting to the shared-memory blocks.
        self.connect_shared_memory()

        # Refresh the impedance display every 500 ms.
        self.root.after(
            500,
            self.update_impedances
        )

    # =====================================================
    # Draw head
    # =====================================================

    def draw_head(self):

        # Head outline
        self.canvas.create_oval(
            100, 60,
            400, 430,
            outline="black",
            width=3
        )

        # Simple nose marker
        self.canvas.create_polygon(
            235, 60,
            265, 60,
            250, 35,
            outline="black",
            fill="white",
            width=2
        )

        self.canvas.create_text(
            250,
            20,
            text="FRONT",
            font=("Arial", 12, "bold")
        )

        self.canvas.create_text(
            250,
            450,
            text="BACK",
            font=("Arial", 12, "bold")
        )

    # =====================================================
    # Draw electrodes
    # =====================================================

    def draw_electrodes(self):

        radius = 25

        for name, (x, y) in ELECTRODES.items():

            circle = self.canvas.create_oval(
                x - radius,
                y - radius,
                x + radius,
                y + radius,
                fill="gray",
                outline="black",
                width=2
            )

            self.canvas.create_text(
                x,
                y,
                text=name,
                font=("Arial", 12, "bold")
            )

            value_text = self.canvas.create_text(
                x,
                y + 40,
                text="--",
                font=("Arial", 10)
            )

            self.electrode_circles[name] = circle
            self.electrode_values[name] = value_text

    # =====================================================
    # Draw legend
    # =====================================================

    def draw_legend(self):

        y = 485

        # Good
        self.canvas.create_oval(
            60,
            y - 10,
            80,
            y + 10,
            fill="green"
        )

        self.canvas.create_text(
            125,
            y,
            text="< 1 MΩ"
        )

        # Check
        self.canvas.create_oval(
            200,
            y - 10,
            220,
            y + 10,
            fill="gold"
        )

        self.canvas.create_text(
            270,
            y,
            text="1–2 MΩ"
        )

        # Poor
        self.canvas.create_oval(
            355,
            y - 10,
            375,
            y + 10,
            fill="red"
        )

        self.canvas.create_text(
            425,
            y,
            text="> 2 MΩ"
        )

    # =====================================================
    # Shared memory connection
    # =====================================================

    def connect_shared_memory(self):

        # If both are already connected, do nothing.
        if (
            self.impedance_memory is not None
            and self.control_memory is not None
        ):
            return

        try:

            self.impedance_memory = shared_memory.SharedMemory(
                name=IMPEDANCE_SHM_NAME,
                create=False
            )

            self.control_memory = shared_memory.SharedMemory(
                name=CONTROL_SHM_NAME,
                create=False
            )

            self.status_label.config(
                text="DSI impedance check active"
            )

            self.start_button.config(
                state=tk.NORMAL
            )

        except FileNotFoundError:

            self.impedance_memory = None
            self.control_memory = None

            self.status_label.config(
                text="Waiting for TinyBCI..."
            )

    # =====================================================
    # Read impedance values from shared memory
    # =====================================================

    def read_impedances(self):

        if self.impedance_memory is None:
            return {}

        try:

            raw = bytes(
                self.impedance_memory.buf[
                    :IMPEDANCE_SHM_SIZE
                ]
            )

            # C clears the unused part of the buffer with
            # null bytes. Keep only the actual string.
            raw = raw.split(b"\x00", 1)[0]

            if not raw:
                return {}

            text = raw.decode("utf-8")

            # Expected format:
            #
            # F3=420000,F4=610000,C3=850000,...
            #
            values = {}

            for item in text.split(","):

                if "=" not in item:
                    continue

                name, value = item.split(
                    "=",
                    1
                )

                name = name.strip()

                try:

                    values[name] = float(value)

                except ValueError:

                    continue

            return values

        except Exception as error:

            print(
                "Error reading impedance:",
                error
            )

            return {}

    # =====================================================
    # Decide colour from impedance
    # =====================================================

    def get_colour(self, impedance):
        if impedance < 0:
            return "gray"

        if impedance < GOOD_THRESHOLD:
            return "green"

        if impedance < CHECK_THRESHOLD:
            return "gold"

        return "red"

    # =====================================================
    # Update UI
    # =====================================================

    def update_impedances(self):

        # If TinyBCI wasn't running when the UI opened,
        # keep trying to connect.
        if self.impedance_memory is None:

            self.connect_shared_memory()

        else:

            values = self.read_impedances()

            for electrode in ELECTRODES:

                if electrode not in values:
                    continue

                impedance = values[electrode]

                colour = self.get_colour(
                    impedance
                )

                self.canvas.itemconfig(
                    self.electrode_circles[electrode],
                    fill=colour
                )
                
                if impedance < 0:
                    value_text = "No reading"
                else:
                    impedance_mohm = (impedance / 1_000_000)
                    value_text = f"{impedance_mohm:.2f} MΩ"
            

                self.canvas.itemconfig(
                    self.electrode_values[electrode],
                    text=value_text
                )

            if values:

                self.status_label.config(
                    text="Impedance monitoring active"
                )

        # Run this function again after 500 ms.
        self.root.after(
            500,
            self.update_impedances
        )

    # =====================================================
    # Start BCI session
    # =====================================================

    def start_session(self):

        if self.control_memory is None:
            return

        # Signal the C program:
        #
        # 0 = continue impedance checking
        # 1 = finish impedance checking and start EEG
        #
        self.control_memory.buf[0] = 1

        self.status_label.config(
            text="Starting BCI session..."
        )

        self.start_button.config(
            state=tk.DISABLED
        )

        # Give TinyBCI a short moment to read the flag,
        # then close this setup window.
        self.root.after(
            500,
            self.root.destroy
        )


# =========================================================
# Main
# =========================================================

def main():

    root = tk.Tk()

    ImpedanceUI(root)

    root.mainloop()


if __name__ == "__main__":
    main()

