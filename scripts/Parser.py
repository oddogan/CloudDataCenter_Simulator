import sys
import numpy as np
import matplotlib.pyplot as plt
from PyQt6.QtWidgets import QApplication, QMainWindow, QFileDialog, QPushButton, QListWidget, QVBoxLayout, QWidget

class DataPlotter(QMainWindow):
    def __init__(self):
        super().__init__()

        self.datasets = []
        self.dataset_labels = []

        self.initUI()

    def initUI(self):
        self.setWindowTitle("Dataset Comparison Tool")
        self.setGeometry(100, 100, 800, 600)

        layout = QVBoxLayout()

        # Button to load binary files
        self.load_button = QPushButton("Load Binary File(s)")
        self.load_button.clicked.connect(self.load_files)
        layout.addWidget(self.load_button)

        # List of loaded files
        self.file_list = QListWidget()
        layout.addWidget(self.file_list)

        # Button to plot data
        self.plot_button = QPushButton("Plot Data")
        self.plot_button.clicked.connect(self.plot_data)
        layout.addWidget(self.plot_button)

        # Main widget
        container = QWidget()
        container.setLayout(layout)
        self.setCentralWidget(container)

    def load_files(self):
        filenames, _ = QFileDialog.getOpenFileNames(self, "Select Binary Files", "", "Binary Files (*.bin)")

        for filename in filenames:
            data = self.parse_binary(filename)
            if data is not None and len(data) > 0:
                self.datasets.append(data)
                self.dataset_labels.append(filename.split("/")[-1])  # Use filename as label
                self.file_list.addItem(filename)

    def parse_binary(self, filename):
        """ Efficiently reads interleaved binary data into structured NumPy arrays. """
        try:
            dtype = np.dtype([
                ('time', 'f8'), ('cpu', 'f8'), ('ram', 'f8'), ('disk', 'f8'),
                ('bandwidth', 'f8'), ('turnedOnMachineCount', 'u8'),
                ('averagePowerConsumption', 'f8'), ('totalPowerConsumption', 'f8'), ('numberOfSLAVs', 'u8')
            ])
            data = np.fromfile(filename, dtype=dtype)
            return data if data.size > 0 else None  # Ensure it's non-empty
        except Exception as e:
            print(f"Error reading {filename}: {e}")
            return None

    def plot_data(self):
        """ Plots multiple datasets on the same figure for easy comparison. """
        if not self.datasets:
            return

        fig, ax = plt.subplots(3, 2, figsize=(12, 8))
        colors = ['b', 'r', 'g', 'c', 'm', 'y', 'k']
        linestyles = ['-', '--', ':', '-.']

        titles = ["Resource Utilization", "Turned On Machines", "SLA Violations",
                  "Average Power Consumption", "Total Power Consumption (kW)"]

        for i, (data, label) in enumerate(zip(self.datasets, self.dataset_labels)):
            color = colors[i % len(colors)]
            linestyle = "-"

            ax[0, 0].plot(data['time'], data['cpu'], linestyle, color=color, label=f"{label} - CPU", alpha=0.7, linewidth=0.8)
            ax[0, 0].plot(data['time'], data['ram'], linestyle, color=color, label=f"{label} - RAM", alpha=0.7, linewidth=0.8)
            ax[0, 0].plot(data['time'], data['disk'], linestyle, color=color, label=f"{label} - Disk", alpha=0.7, linewidth=0.8)
            ax[0, 0].plot(data['time'], data['bandwidth'], linestyle, color=color, label=f"{label} - Bandwidth", alpha=0.7, linewidth=0.8)

            ax[0, 1].plot(data['time'], data['turnedOnMachineCount'], linestyle, color=color, label=f"{label} - Machines", alpha=0.7, linewidth=0.8)
            ax[1, 0].plot(data['time'], data['numberOfSLAVs'], linestyle, color=color, label=f"{label} - SLA Violations", alpha=0.7, linewidth=0.8)
            ax[1, 1].plot(data['time'], data['averagePowerConsumption'], linestyle, color=color, label=f"{label} - Avg Power", alpha=0.7, linewidth=0.8)
            ax[2, 0].plot(data['time'], data['totalPowerConsumption'] / 1e3, linestyle, color=color, label=f"{label} - Total Power (kW)", alpha=0.7, linewidth=0.8)
        
        # Calculate total power consumption difference between two datasets if there are two
        # The dataset length is checked to avoid out-of-range errors
        # Indicate which dataset is subtracted by the color of the line
        # The common length of the two datasets is used to calculate the difference
        if len(self.datasets) == 2:
            common_length = min(len(self.datasets[0]), len(self.datasets[1]))
            difference = self.datasets[0][:common_length]['totalPowerConsumption'] - self.datasets[1][:common_length]['totalPowerConsumption']
            # For each point: If the difference is positive, red color, If the difference is negative, green color, usable format for the color parameter of the plot function
            color = ['r' if diff > 0 else 'g' for diff in difference]
            # Plot each point according to its color
            ax[2, 1].scatter(self.datasets[0][:common_length]['time'], difference / 1e3, color=color, label=f"{self.dataset_labels[0]} - {self.dataset_labels[1]}", alpha=0.7, linewidth=0.8)
            # Print the title of the plot as the difference between the two datasets
            titles[4] = f"Total Power Consumption Difference ({self.dataset_labels[0]} - {self.dataset_labels[1]})"

        

        # Set subplot titles correctly and avoid out-of-range errors
        for i in range(3):
            for j in range(2):
                if i * 2 + j < len(titles):  # Ensure we don't exceed titles list size
                    ax[i, j].set_title(titles[i * 2 + j])
                ax[i, j].grid()

                # Only show legend if there are labels
                handles, labels = ax[i, j].get_legend_handles_labels()
                if handles:
                    ax[i, j].legend(loc="lower right")

        plt.tight_layout()
        plt.show()

# Run the application
if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = DataPlotter()
    window.show()
    sys.exit(app.exec())