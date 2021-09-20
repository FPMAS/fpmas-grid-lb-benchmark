import matplotlib
import matplotlib.pyplot as plt
import sys
import json
import argparse
import pathlib
import random as rd

def plot_grid(grid):
    plt.title("Utility")
    plt.pcolormesh(grid)
    plt.tight_layout()

def plot_cells(cells):
    plt.title("Cells location")
    plt.pcolormesh(cells)
    def on_resize(event):
        plt.figure(1).tight_layout()
        plt.figure(1).canvas.draw()

    cid = plt.figure(1).canvas.mpl_connect('resize_event', on_resize)

def build_graph_lines(agents, sample_size):
    lines = []
    agent_loc_index = {}
    for agent in agents:
        agent_loc_index[str(agent["id"])] = agent["location"]
    print(sample_size)
    indexes = rd.sample(range(len(agents)), sample_size)
    for index in indexes:
        agent = agents[index]
        for contact_id in agent["contacts"]:
            contact_location = agent_loc_index[str(contact_id)]
            lines.append(matplotlib.lines.Line2D(
                [agent["location"][0]+.5, contact_location[0]+.5],
                [agent["location"][1]+.5, contact_location[1]+.5]
                ))
    return lines

def plot_agents(agent_files, draw_graph, sample_size):
    agent_counts = []
    agent_graphs = []
    for agent_file in agent_files:
        with open(agent_file) as json_file:
            agents_output = json.load(json_file)
            agent_grid = [
                    [0 for _ in range(0, agents_output["grid"]["height"])]
                    for _ in range(0, agents_output["grid"]["width"])
                    ]
            agent_counts.append(agent_grid)
            for agent in agents_output["agents"]:
                agent_grid[agent["location"][1]][agent["location"][0]]+=1
            if draw_graph:
                agent_graphs.append(
                        build_graph_lines(agents_output["agents"], sample_size)
                        )

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
        n = int(len(config.agent_files) / m)
    elif config.m is None:
        n = config.n
        m = int(len(config.agent_files) / n)

    vmax = max([max([max(row) for row in matrix]) for matrix in
        agent_counts])

    fig, axes = plt.subplots(m, n, sharex=True, sharey=True,
            squeeze=False)
    i = 0
    for x in range(m):
        for y in range(n):
            if(i >= len(agent_counts)):
                break
            axes[x][y].pcolormesh(agent_counts[i], cmap="Reds", vmin=0, vmax=vmax)
            if draw_graph:
                for line in agent_graphs[i]:
                    axes[x][y].add_line(line)
            i+=1

    def on_resize(event):
        fig.tight_layout()
        fig.canvas.draw()

    cid = fig.canvas.mpl_connect('resize_event', on_resize)

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
            '-s', '--graph-sample', type=int, nargs='?', const=20, default=None,
            help="Sample size to draw graph lines. "
            "If not specified, no graph is drawn. "
            "If specified without arguments, a default value of 20 is used."
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
    print(config)
    if config.grid_file is not None:
        with open(config.grid_file) as json_file:
            grid = json.load(json_file)
            plot_grid(grid)
    if config.cell_file is not None:
        with open(config.cell_file) as json_file:
            plt.figure()
            cells = json.load(json_file)
            plot_cells(cells)
    if config.agent_files is not None:
        plot_agents(
                config.agent_files,
                True if config.graph_sample is not None else False,
                config.graph_sample
                )

    plt.show()
