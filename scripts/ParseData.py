import numpy as np
import matplotlib.pyplot as plt

def parse_binary(filename):
    # Define the structure: 10 doubles per record (each 8 bytes)
    dtype = np.dtype([
        ('time', 'f8'), ('cpu', 'f8'), ('ram', 'f8'), ('disk', 'f8'),
        ('bandwidth', 'f8'), ('fpga', 'f8'), ('turnedOnMachineCount', 'u8'),
        ('averagePowerConsumption', 'f8'), ('totalPowerConsumption', 'f8'), ('numberOfSLAVs', 'u8')
    ])
    
    # Read binary data efficiently
    data = np.fromfile(filename, dtype=dtype)
    
    return data

def plot_data(data):
    fig, ax = plt.subplots(3, 2, figsize=(12, 8))

    ax[0, 0].plot(data['time'], data['cpu'], label="CPU")
    ax[0, 0].plot(data['time'], data['ram'], label="RAM")
    ax[0, 0].plot(data['time'], data['disk'], label="Disk")
    ax[0, 0].plot(data['time'], data['bandwidth'], label="Bandwidth")
    ax[0, 0].plot(data['time'], data['fpga'], label="FPGA")
    ax[0, 0].set_title("Resource Utilization")
    ax[0, 0].legend()
    
    ax[1, 0].plot(data['time'], data['turnedOnMachineCount'], label="Machines")
    ax[1, 0].set_title("Turned On Machines")
    
    ax[1, 1].plot(data['time'], data['numberOfSLAVs'], label="SLA Violations")
    ax[1, 1].set_title("SLA Violations")
    
    ax[2, 0].plot(data['time'], data['averagePowerConsumption'], label="Avg Power Consumption")
    ax[2, 0].set_title("Average Power Consumption")
    
    ax[2, 1].plot(data['time'], data['totalPowerConsumption'] / 1e3, label="Total Power (kW)")
    ax[2, 1].set_title("Total Power Consumption")
    
    for i in range(3):
        for j in range(2):
            ax[i, j].grid()
            ax[i, j].legend()
    
    plt.tight_layout()
    plt.show()

# Load data
data = parse_binary("/Users/oddogan/MSc/CDC/CloudDataCenter_Simulator/runs/ILPDQN128_10e3.bin")

plot_data(data);