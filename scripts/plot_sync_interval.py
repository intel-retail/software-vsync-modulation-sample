#!/usr/bin/env python3
# *
# * Copyright © 2024 Intel Corporation
# *
# * Permission is hereby granted, free of charge, to any person obtaining a
# * copy of this software and associated documentation files (the "Software"),
# * to deal in the Software without restriction, including without limitation
# * the rights to use, copy, modify, merge, publish, distribute, sublicense,
# * and/or sell copies of the Software, and to permit persons to whom the
# * Software is furnished to do so, subject to the following conditions:
# *
# * The above copyright notice and this permission notice (including the next
# * paragraph) shall be included in all copies or substantial portions of the
# * Software.
# *
# * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# * IN THE SOFTWARE.
#
"""
chrony_sync_plot.py

This script reads a CSV file containing Software GenLock synchronization data 
generated using Chrony. It parses the configuration metadata (prefixed with '#') 
from the beginning of the file and uses the data to generate a line chart.

Features:
- Cleans and parses time values in the format [+1234.56s]
- Displays synchronization intervals over time with annotated data points
- Converts timeline seconds to readable time format (e.g., 1h 2m)
- Adds a horizontal trend line to represent the configured learning period
- Saves the plot as a high-resolution PNG image

Usage:
    python chrony_sync_plot.py path/to/data.csv

Author: Arshad Mehmood
Date: June 2025
"""

import csv
import matplotlib.pyplot as plt
import textwrap
from matplotlib.ticker import FuncFormatter
import argparse
import os

def format_x_ticks(x, pos):
    minutes = int(x) // 60
    seconds = int(x) % 60
    return f"{int(x)} ({minutes}m {seconds}s)"
    
def clean_time_string(time_str):
    """Extract integer seconds from a string like [+3232s]"""
    try:
        return round(float(time_str.strip().lstrip('[+').rstrip('s]')))
    except ValueError:
        return None

def main():
    parser = argparse.ArgumentParser(description='Plot line chart from CSV data.')
    parser.add_argument('csv_file', help='Path to the CSV file')
    args = parser.parse_args()

    csv_file = args.csv_file

    if not os.path.isfile(csv_file):
        print(f"Error: File not found: {csv_file}")
        return

    subtitle_lines = []
    with open(csv_file, 'r') as file:
        for line in file:
            if line.startswith('#\t') or line.startswith('# '):
                line = line.lstrip('#').strip()
                if ':' in line:
                    key, value = line.split(':', 1)
                    key, value = key.strip(), value.strip()
                    subtitle_lines.append(f"{key}: {value}")
                    if key.lower() == 'time_period':
                        try:
                            time_period = float(value.split()[0])
                        except ValueError:
                            pass
            elif line.startswith('#[+'):
                continue
            elif not line.startswith('#'):
                break


    subtitle_text = ' | '.join(subtitle_lines)

    time_seconds = []
    y_values = []

    with open(csv_file, 'r') as file:
        reader = csv.reader(file)
        for row in reader:
            if not row or row[0].startswith('#') or len(row) < 2:
                continue
            
            time_sec = clean_time_string(row[0])
            try:
                y_val = float(row[1].strip())
            except ValueError:
                continue
            if time_sec is not None:
                time_seconds.append(time_sec)
                y_values.append(y_val)

    if not time_seconds:
        print("No valid data found.")
        return
    
    def format_x_ticks(x, pos):
        total_seconds = int(x)
        hours = total_seconds // 3600
        minutes = (total_seconds % 3600) // 60
        seconds = total_seconds % 60
        parts = []
        
        if hours > 0:
            parts.append(f"{hours}h")
        if minutes > 0:
            parts.append(f"{minutes}m")
        if seconds > 0 and hours == 0:  # include seconds only if less than 1 hour
            parts.append(f"{seconds}s")
        return ' '.join(parts)

    # Create figure and axis
    fig, ax = plt.subplots(figsize=(10, 6))
    ax.plot(time_seconds, y_values, marker='o', linestyle='-')
    fig.suptitle('Software GenLock Runs (Chrony)', fontsize=14, y=0.98)
    wrapped_subtitle = "\n".join(textwrap.wrap(subtitle_text, width=200))
    ax.set_title(wrapped_subtitle, fontsize=9, loc='center')
    # Labels
    ax.set_xlabel('Timeline (From start of program)')
    ax.set_ylabel('Duration between each sync')
    if time_period:
        plt.axhline(y=time_period, color='blue', linestyle='--', linewidth=1.2,          label=f'Learning Period = {int(time_period)}s')
        # Place annotation slightly below the line near the end of x-axis
        plt.text(
            time_seconds[-1],               # x-position at the end of data
            time_period - 20,               # y-position slightly below the line
            f'Learning Period Range = {int(time_period)}s',
            color='blue',
            fontsize=8,
            ha='right',
            va='top'
        )
        '''
        plt.text(x=time_seconds[-1], y=time_period - 20 , s=f'Learning Period Range: {format_x_ticks(time_period,0)} ({int(time_period)}s)',
             color='blue', ha='right', fontsize=10)
        '''
    plt.grid(True)
    plt.tight_layout(rect=[0.03, 0.05, 1, 0.92])
    
    plt.gca().yaxis.set_major_formatter(FuncFormatter(format_x_ticks))
    plt.gca().xaxis.set_major_formatter(FuncFormatter(format_x_ticks))
    
    # Derive output PNG file name from input CSV file name
    output_file = os.path.splitext(csv_file)[0] + ".png"
    plt.savefig(output_file, dpi=600, bbox_inches='tight')
    
    plt.show()

if __name__ == '__main__':
    main()
