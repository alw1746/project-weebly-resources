import pandas as pd
import numpy as np

def generate_battery_lut(csv_file, max_v, min_v):
    """
    Generates C++ Voltage and Current Lookup Tables for any battery chemistry.
    
    :param csv_file: Path to the CSV log file (Must have 'mahConsumed' and 'Voltage' columns)
    :param max_v: The voltage to define as 100% (e.g., 4.2, 9.6, 3.65)
    :param min_v: The voltage to define as 0% (e.g., 3.0, 5.4, 2.5)
    """
    
    print(f"Processing {csv_file} for range {min_v}V - {max_v}V...")

    # 1. Load Data
    try:
        df = pd.read_csv(csv_file)
    except FileNotFoundError:
        print(f"Error: Could not find '{csv_file}'")
        return

    # Verify required columns exist
    if 'mahConsumed' not in df.columns or 'Voltage' not in df.columns:
        print("Error: CSV must contain 'mahConsumed' and 'Voltage' headers.")
        return

    # 2. Smooth Data
    # Rolling mean smooths out transient dips from load spikes
    df['Voltage_Smooth'] = df['Voltage'].rolling(window=20, min_periods=1, center=True).mean()

    # 3. Find the 0% Cutoff Point (defined by min_v)
    cutoff_mask = df['Voltage_Smooth'] <= min_v
    if cutoff_mask.any():
        end_idx = cutoff_mask.idxmax()
    else:
        end_idx = len(df) - 1
        print(f"Warning: Battery never reached {min_v}V. Using last data point.")

    # Determine maximum mAh consumed at the cutoff voltage
    max_mah = df.loc[end_idx, 'mahConsumed']
    print(f"100% Consumed point found at index {end_idx} ({df.loc[end_idx, 'Voltage']:.3f}V, {max_mah:.1f} mAh total)")

    # 4. Enforce Monotonicity on Voltage
    # Ensure voltage only ever goes down as it discharges
    discharge_slice_v = df.loc[0:end_idx, 'Voltage_Smooth']
    voltage_curve = np.minimum.accumulate(discharge_slice_v)
    mah_curve = df.loc[0:end_idx, 'mahConsumed']

    # 5. Generate LUTs
    lut_v = {}
    lut_c = {}

    for percent in range(101):
        # Calculate target mAh for the given percentage.
        # target_mah goes from 0 to max_mah as percent goes from 0 to 100
        target_mah = max_mah * (percent / 100.0) 
        lut_c[percent] = target_mah

        # Map exact voltages to percentages
        if percent == 100:
            lut_v[percent] = min_v  # FIX: 100% consumed = minimum voltage
        elif percent == 0:
            lut_v[percent] = max_v  # FIX: 0% consumed = maximum voltage
        else:
            # Interpolate the voltage curve based on the target mAh
            lut_v[percent] = np.interp(target_mah, mah_curve, voltage_curve)

    # FIX: Safety: Ensure Voltage LUT is strictly descending
    # (Because index is consumed capacity %, higher index = lower voltage)
    for p in range(1, 101):
        if lut_v[p] > lut_v[p-1]:
            lut_v[p] = lut_v[p-1]

    # 6. Output C++ Code
    print("\n" + "="*40)
    print("   GENERATED C++ CODE")
    print("="*40 + "\n")

    # Function 1: Voltage Table
    print("void initVoltageTable() {")
    for p in range(101):
        # formatted with >3 for clean vertical alignment
        print(f"    vt[{p:>3}] = {lut_v[p]:.3f};")
    print("}\n")

    # Function 2: Current (Capacity) Table
    print("void initCurrentTable() {")
    for p in range(101):
        # formatted to 1 decimal place for mAh precision
        print(f"    ct[{p:>3}] = {lut_c[p]:.1f};")
    print("}")

# --- CONFIGURATION SECTION ---
if __name__ == "__main__":
    # Ensure your CSV has headers exactly matching: 'mahConsumed' and 'Voltage'
    generate_battery_lut('battery3.csv', max_v=4.20, min_v=3.5)
