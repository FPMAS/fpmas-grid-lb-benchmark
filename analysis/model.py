import matplotlib.pyplot as plt
import sys
import json
import argparse
import pathlib

def plot_grid(grid):
    plt.title("Grid shape")
    plt.pcolormesh(grid)

def plot_cells(cells):
    plt.title("Cells location")
    plt.pcolormesh(cells)

def plot_agents(agent_files):
    agent_counts = []
    for agent_file in config.agent_files:
        with open(agent_file) as json_file:
            agent_counts.append(json.load(json_file))
    n = 0
    m = 0
    if config.n is None and config.m is None:
        n = len(config.agent_files)
        m = 1
    elif config.n is not None and config.m is not None:
        n = config.n
        m = config.m
    elif config.n is None:
        m = config.m
        n = len(config.agent_files) / m
    elif config.m is None:
        n = config.n
        m = len(config.agent_files) / n

    vmax = max([max([max(row) for row in matrix]) for matrix in
        agent_counts])
    i = 1
    for agent_count in agent_counts:
        plt.subplot(m, n, i)
        plt.pcolormesh(agent_count, cmap="Reds", vmin=0, vmax=vmax)
        i+=1

def build_arg_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument(
            '-g', '--grid-file', type=pathlib.Path,
            help="Grid shape json file"
            )
    parser.add_argument(
            '-c', '--cell-file', type=pathlib.Path,
            help="Cells distribution json file"
            )
    parser.add_argument(
            '-a', '--agent-files', type=pathlib.Path, nargs='*',
            help="Local agent counts for each process"
            )
    parser.add_argument(
            '-n', type=int,
            help="Number of columns to display agent counts"
            )
    parser.add_argument(
            '-m', type=int,
            help="Number of lines to display agent counts"
            )
    return parser

if __name__ == "__main__":
    config = build_arg_parser().parse_args();
    if config.grid_file is not None:
        with open(config.grid_file) as json_file:
            grid = json.load(json_file)
            plot_grid(grid)
            plt.figure()
    if config.cell_file is not None:
        with open(config.cell_file) as json_file:
            cells = json.load(json_file)
            plot_cells(cells)
            plt.figure()
    if config.agent_files is not None:
        plot_agents(config.agent_files)

    plt.show()
