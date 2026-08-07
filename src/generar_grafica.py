import re
import os
import matplotlib
matplotlib.use('Agg') # Evita problemas de GDK/X11 en entornos sin servidor grafico
import matplotlib.pyplot as plt

def parse_trec_res(filepath):
    """
    Parses a trec_eval output file and extracts the Interpolated Recall - Precision
    Averages for the 'All' query summary.
    """
    print(f"Parsing {filepath}...")
    if not os.path.exists(filepath):
        print(f"Error: {filepath} not found.")
        return None
        
    with open(filepath, 'r') as f:
        content = f.read()
        
    # Split into query sections
    sections = content.split("Queryid (Num):")
    all_section = None
    for sec in sections:
        if sec.strip().startswith("All"):
            all_section = sec
            break
            
    if not all_section:
        print(f"Error: Could not find 'All' summary section in {filepath}.")
        return None
        
    # Extract recall-precision values
    # Format typically: "    at 0.10       0.7094"
    pattern = re.compile(r"at\s+([0-9.]+)\s+([0-9.]+)")
    recall_levels = []
    precision_values = []
    
    # We only want the Interpolated Recall - Precision Averages block
    # which comes after the header and before "Average precision"
    lines = all_section.split("\n")
    in_block = False
    for line in lines:
        if "Interpolated Recall - Precision Averages" in line:
            in_block = True
            continue
        if in_block and "Average precision" in line:
            in_block = False
            break
        if in_block:
            match = pattern.search(line)
            if match:
                recall_levels.append(float(match.group(1)))
                precision_values.append(float(match.group(2)))
                
    return recall_levels, precision_values

def main():
    configs = {
        'DFR (con stemming)': 'trec_DFR_stem.res',
        'BM25 (con stemming)': 'trec_BM25_stem.res',
        'DFR (sin stemming)': 'trec_DFR_nostem.res',
        'BM25 (sin stemming)': 'trec_BM25_nostem.res',
    }
    
    data = {}
    recall_levels = None
    
    for label, filename in configs.items():
        res = parse_trec_res(filename)
        if res:
            levels, precisions = res
            if recall_levels is None:
                recall_levels = levels
            data[label] = precisions

    if not data or recall_levels is None:
        print("Error: No data successfully parsed. Check trec_*.res files.")
        return

    # 1. Save to CSV for easy copy-paste / Excel import
    csv_path = 'datos_grafica.csv'
    with open(csv_path, 'w') as f:
        # Header
        f.write("Recall," + ",".join(data.keys()) + "\n")
        # Rows
        for i, r_level in enumerate(recall_levels):
            row_vals = [f"{r_level:.2f}"]
            for label in data.keys():
                row_vals.append(f"{data[label][i]:.4f}")
            f.write(",".join(row_vals) + "\n")
    print(f"CSV exported successfully to: {csv_path}")

    # 2. Plotting with matplotlib
    plt.figure(figsize=(10, 7))
    
    # Curated premium color palette
    colors = {
        'DFR (con stemming)': '#1f77b4',   # Premium Blue
        'BM25 (con stemming)': '#ff7f0e',  # Premium Orange
        'DFR (sin stemming)': '#2ca02c',  # Premium Green
        'BM25 (sin stemming)': '#d62728', # Premium Red
    }
    
    # Markers for lines
    markers = {
        'DFR (con stemming)': 'o',
        'BM25 (con stemming)': 's',
        'DFR (sin stemming)': '^',
        'BM25 (sin stemming)': 'D',
    }

    for label, precisions in data.items():
        plt.plot(recall_levels, precisions, label=label, color=colors[label], 
                 marker=markers[label], linewidth=2.5, markersize=7)

    plt.title('Gráfica Precisión-Cobertura (Corpus TIME)', fontsize=14, fontweight='bold', pad=15)
    plt.xlabel('Cobertura (Recall)', fontsize=12, fontweight='bold', labelpad=10)
    plt.ylabel('Precisión', fontsize=12, fontweight='bold', labelpad=10)
    
    plt.xlim(-0.02, 1.02)
    plt.ylim(-0.02, 1.02)
    plt.xticks([i/10 for i in range(11)])
    plt.yticks([i/10 for i in range(11)])
    
    plt.grid(True, linestyle='--', alpha=0.5)
    plt.legend(fontsize=11, loc='upper right', frameon=True, facecolor='white', edgecolor='#e0e0e0')
    
    # Clean styling
    plt.gca().spines['top'].set_visible(False)
    plt.gca().spines['right'].set_visible(False)
    plt.tight_layout()
    
    image_path = 'src/graficaPrecisionCobertura.png'
    plt.savefig(image_path, dpi=300)
    print(f"Graph image saved successfully to: {image_path}")

if __name__ == '__main__':
    main()
