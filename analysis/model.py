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

def build_agent_location_index(agent_files):
    agent_loc_index = {}
    for agent_file in agent_files:
        with open(agent_file) as json_file:
            agents_output = json.load(json_file)
            local_agent_index = {}
            for agent in agents_output["agents"]:
                local_agent_index[str(agent["id"])] = agent["location"]

            agent_loc_index[agents_output["rank"]] = local_agent_index
    return agent_loc_index


def build_graph_lines(agent_output, agent_location_index, sample_size):
    """
    Builds a matplotlib.lines.Line2D list containing representations of agent
    contacts.

    The matplotlib.axes.Axes.add_line() method can then be used to plot those
    line on a figure.

    Only the contacts of an agents sample of size `sample_size`, randomly
    selected among the available local agents, is represented.

    The `agent_location_index` is a map of shape `{process_rank: {agent_id: [x, y]}}`
    that contains the location of **all** agents in the simulation. It is used
    to get the location of distant agents on the local process (accessible
    through `agent_output["distant_agents"]`. Indeed, the only information
    available for distant agents are their id and the rank of the process that
    own them.
    """
    lines = []
    distant_agents = {str(agent["id"]): agent["rank"] for agent in
            agent_output["distant_agents"]}
    indexes = rd.sample(range(len(agent_output["agents"])), sample_size)
    for index in indexes:
        agent = agent_output["agents"][index]
        for contact_id in agent["contacts"]:
            contact_location = []
            if str(contact_id) in distant_agents:
                contact_location = agent_location_index[
                        distant_agents[str(contact_id)]
                        ][str(contact_id)]
            else:
                contact_location = agent_location_index[
                        agent_output["rank"]
                        ][str(contact_id)]
            lines.append(matplotlib.lines.Line2D(
                [agent["location"][0]+.5, contact_location[0]+.5],
                [agent["location"][1]+.5, contact_location[1]+.5]
                ))
    return lines

def alpha_color_map(base_color):
    n_colors=256
    colors = [(base_color[0], base_color[1], base_color[2], float(i)/n_colors) for i in
            range(n_colors)]
    cmap = matplotlib.colors.ListedColormap(colors)
    return cmap

def build_null_grid(agents_output):
    """
    Builds a matrix representing the current `agents_output`, with null
    coefficients.
    """
    return [
            [0 for _ in range(0, agents_output["grid"]["height"])]
            for _ in range(0, agents_output["grid"]["width"])
            ]

def plot_agents(agent_files, draw_graph, sample_size):
    agent_counts = []
    distant_agent_counts = []
    agent_graphs = []
    agent_location_index = build_agent_location_index(agent_files)
    for agent_file in agent_files:
        with open(agent_file) as json_file:
            agents_output = json.load(json_file)
            agent_grid = build_null_grid(agents_output)
            distant_agent_grid = build_null_grid(agents_output)

            agent_counts.append(agent_grid)
            distant_agent_counts.append(distant_agent_grid)

            for agent in agents_output["agents"]:
                # counts local agents
                agent_grid[agent["location"][1]][agent["location"][0]]+=1
            for distant_agent in agents_output["distant_agents"]:
                # counts distant agents
                location = agent_location_index[distant_agent["rank"]][str(distant_agent["id"])]
                distant_agent_grid[location[1]][location[0]]+=1

            if draw_graph:
                agent_graphs.append(
                        build_graph_lines(
                            agents_output, agent_location_index, sample_size
                            )
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

    local_vmax = max([max([max(row) for row in matrix]) for matrix in
        agent_counts])
    distant_vmax = max([max([max(row) for row in matrix]) for matrix in
        distant_agent_counts])
    vmax = max([local_vmax, distant_vmax])

    fig, axes = plt.subplots(m, n, sharex=True, sharey=True,
            squeeze=False)
    local_cmap = alpha_color_map([1., 0., 0.])
    distant_cmap = alpha_color_map([0., 0., 1.])
    i = 0
    for x in range(m):
        for y in range(n):
            if(i >= len(agent_counts)):
                break
            axes[x][y].pcolormesh(
                    agent_counts[i], cmap=local_cmap, vmin=0,
                    vmax=vmax)
            axes[x][y].pcolormesh(
                    distant_agent_counts[i], cmap=distant_cmap, vmin=0,
                    vmax=vmax)
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
