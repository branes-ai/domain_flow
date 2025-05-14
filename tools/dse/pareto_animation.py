import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import matplotlib.ticker as ticker
import os

def is_pareto_efficient(costs):
    """
    Find the Pareto-efficient points
    :param costs: An (n_points, n_costs) array
    :return: A boolean array of shape (n_points,), indicating whether each point is Pareto efficient
    """
    is_efficient = np.ones(costs.shape[0], dtype=bool)
    for i, c in enumerate(costs):
        if is_efficient[i]:
            # Keep any point with a lower cost in at least one dimension
            is_efficient[is_efficient] = np.any(costs[is_efficient] < c, axis=1) | np.all(costs[is_efficient] == c, axis=1)
    return is_efficient

def read_data(filepath):
    """Read latency and energy data from file"""
    # Assuming CSV format with headers 'latency_us' and 'energy_pj'
    try:
        df = pd.read_csv(filepath)
        latency = df['latency_us'].values
        energy = df['energy_pj'].values
        return latency, energy
    except Exception as e:
        print(f"Error reading data file: {e}")
        # Generate sample data if file reading fails
        print("Generating sample data instead...")
        np.random.seed(42)
        latency = np.random.uniform(1, 100, 50)
        energy = np.random.uniform(1, 100, 50)
        # Ensure there's a tradeoff relationship
        for i in range(len(latency)):
            energy[i] += np.random.uniform(0, 100/latency[i])
        return latency, energy

def create_pareto_animation(latency, energy, output_filename='pareto_animation.mp4'):
    """Create animation of points being added and Pareto frontier forming"""
    # Find the minimum latency and energy point (speed of light / minimum energy)
    min_latency_idx = np.argmin(latency)
    min_energy_idx = np.argmin(energy)
    
    # Combine the data and sort by the sum of normalized values to get a reasonable traversal order
    norm_latency = (latency - np.min(latency)) / (np.max(latency) - np.min(latency))
    norm_energy = (energy - np.min(energy)) / (np.max(energy) - np.min(energy))
    efficiency = norm_latency + norm_energy
    
    # Get sorted indices (we'll start with most efficient point)
    sorted_indices = np.argsort(efficiency)
    
    # Setup the figure and axes
    fig, ax = plt.subplots(figsize=(10, 6))
    
    # Set a reasonable axis scale with some padding
    x_padding = 0.1 * (np.max(latency) - np.min(latency))
    y_padding = 0.1 * (np.max(energy) - np.min(energy))
    ax.set_xlim(np.min(latency) - x_padding, np.max(latency) + x_padding)
    ax.set_ylim(np.min(energy) - y_padding, np.max(energy) + y_padding)
    
    # Add labels and title
    ax.set_xlabel('Latency (μs)')
    ax.set_ylabel('Energy (pJ)')
    ax.set_title('Pareto Frontier Evolution')
    
    # Use scientific notation for large numbers
    ax.xaxis.set_major_formatter(ticker.ScalarFormatter(useMathText=True))
    ax.yaxis.set_major_formatter(ticker.ScalarFormatter(useMathText=True))
    ax.ticklabel_format(style='sci', scilimits=(0,0), axis='both')
    
    # Initialize empty scatter plots
    points_scatter = ax.scatter([], [], color='blue', alpha=0.6, label='Data Points')
    frontier_scatter = ax.scatter([], [], color='red', s=80, label='Pareto Frontier')
    
    # Add legend
    ax.legend()
    
    # Initialize empty data lists
    points_x = []
    points_y = []
    frontier_x = []
    frontier_y = []
    
    def init():
        """Initialize animation"""
        points_scatter.set_offsets(np.empty((0, 2)))
        frontier_scatter.set_offsets(np.empty((0, 2)))
        return points_scatter, frontier_scatter
    
    def update(frame):
        """Update function for animation"""
        # Add a new point
        if frame < len(sorted_indices):
            idx = sorted_indices[frame]
            points_x.append(latency[idx])
            points_y.append(energy[idx])
            
            # Update all points scatter
            points_scatter.set_offsets(np.column_stack((points_x, points_y)))
            
            # Find Pareto optimal points from current set
            current_points = np.column_stack((points_x, points_y))
            if len(current_points) > 0:
                pareto_mask = is_pareto_efficient(current_points)
                frontier_points = current_points[pareto_mask]
                
                # Sort frontier points by x value for cleaner visualization
                frontier_points = frontier_points[frontier_points[:, 0].argsort()]
                
                frontier_scatter.set_offsets(frontier_points)
        
        # Add a progress indicator in the title
        ax.set_title(f'Pareto Frontier Evolution: {frame+1}/{len(sorted_indices)} points')
        
        return points_scatter, frontier_scatter
    
    # Create animation
    frames = len(sorted_indices)
    animation = FuncAnimation(
        fig, update, frames=frames, init_func=init, 
        blit=False, interval=200, repeat_delay=2000
    )
    
    # Determine the output format
    output_file = output_filename
    file_extension = os.path.splitext(output_filename)[1].lower()
    
    # If output is a GIF file, use pillow
    if file_extension == '.gif':
        print(f"Saving animation as GIF to {output_file}...")
        animation.save(output_file, writer='pillow', fps=5, dpi=150)
        print(f"Animation saved successfully to {output_file}!")
    else:
        # Try different writers for video formats
        from matplotlib.animation import writers
        
        available_writers = writers.list()
        print(f"Available animation writers: {available_writers}")
        
        if 'ffmpeg' in available_writers:
            print("Using ffmpeg writer...")
            writer = 'ffmpeg'
        elif 'imagemagick' in available_writers:
            print("Using imagemagick writer...")
            writer = 'imagemagick'
        else:
            print("No video writers available. Converting to GIF format...")
            output_file = os.path.splitext(output_filename)[0] + '.gif'
            writer = 'pillow'
        
        try:
            print(f"Saving animation to {output_file}...")
            animation.save(output_file, writer=writer, fps=5, dpi=150)
            print(f"Animation saved successfully to {output_file}!")
        except Exception as e:
            print(f"Error saving animation: {e}")
            print("Falling back to GIF format...")
            output_file = os.path.splitext(output_filename)[0] + '.gif'
            animation.save(output_file, writer='pillow', fps=5, dpi=150)
            print(f"Animation saved as GIF to {output_file} instead.")
    
    plt.close()
    return output_file

def main():
    import argparse
    import subprocess
    import sys
    import platform
    
    parser = argparse.ArgumentParser(description='Generate Pareto Frontier Animation')
    parser.add_argument('--data', type=str, default='pareto_data.csv',
                        help='Path to data file with latency_us and energy_pj columns')
    parser.add_argument('--output', type=str, default='pareto_animation.mp4',
                        help='Output animation filename')
    parser.add_argument('--format', type=str, choices=['mp4', 'gif'], default=None,
                        help='Force output format (mp4 or gif)')
    parser.add_argument('--view', action='store_true',
                        help='Automatically open the animation after creation')
    
    args = parser.parse_args()
    
    # Check if data file exists and create a sample if it doesn't
    if not os.path.exists(args.data):
        print(f"Data file {args.data} not found. Creating a sample data file...")
        # Generate sample data
        np.random.seed(42)
        latency = np.random.uniform(1, 100, 50)
        energy = np.random.uniform(1, 100, 50)
        # Ensure there's a tradeoff relationship
        for i in range(len(latency)):
            energy[i] += np.random.uniform(0, 100/latency[i])
        
        # Save to CSV
        df = pd.DataFrame({'latency_us': latency, 'energy_pj': energy})
        df.to_csv(args.data, index=False)
        print(f"Sample data saved to {args.data}")
    
    print(f"Reading data from {args.data}...")
    latency, energy = read_data(args.data)
    print(f"Found {len(latency)} data points.")
    
    # Force output format if specified
    output_file = args.output
    if args.format:
        base_name, _ = os.path.splitext(args.output)
        output_file = f"{base_name}.{args.format}"
    
    output_file = create_pareto_animation(latency, energy, output_file)
    print(f"Animation complete! Saved to {output_file}")
    
    # Automatically open the file if requested
    if args.view:
        print(f"Opening {output_file}...")
        try:
            if platform.system() == 'Darwin':  # macOS
                subprocess.call(('open', output_file))
            elif platform.system() == 'Windows':
                os.startfile(output_file)
            else:  # Linux
                subprocess.call(('xdg-open', output_file))
            print("File opened successfully!")
        except Exception as e:
            print(f"Error opening file: {e}")
            print(f"Please open {output_file} manually.")

if __name__ == "__main__":
    main()
