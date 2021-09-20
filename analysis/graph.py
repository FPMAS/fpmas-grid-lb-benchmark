import argparse
import pathlib
import json
from graph_tool import *
from graph_tool.stats import *
from graph_tool.clustering import *
from graph_tool.generation import *
import numpy as np
from prettytable import PrettyTable
from prettytable import PLAIN_COLUMNS

def load_graph(agent_files, read_perceptions=False, read_contacts=True):
    id_to_vertex = {}
    json_agent_files = []
    g = Graph()

    for agent_file in agent_files:
        with open(agent_file) as json_file:
            json_agent_files.append(json.load(json_file))

    for agent_file in json_agent_files:
        for agent in agent_file["agents"]:
            v = g.add_vertex()
            id_to_vertex[str(agent["id"])] = v
    for agent_file in json_agent_files:
        for agent in agent_file["agents"]:
            if read_perceptions:
                for perception in agent["perceptions"]:
                    g.add_edge(
                            id_to_vertex[str(agent["id"])],
                            id_to_vertex[str(perception)])
            if read_contacts:
                for contact in agent["contacts"]:
                    g.add_edge(
                            id_to_vertex[str(agent["id"])],
                            id_to_vertex[str(contact)])
    return g

def average_shortest_path_length(graph):
    (counts, bins) = distance_histogram(graph)

    average = 0
    num_vertices = 0
    for i in range(0, len(counts)):
        average += counts[i] * bins[i]
        num_vertices += counts[i]
    return average / num_vertices

def clustering_coefficient(graph):
    clust = local_clustering(graph, undirected=False)
    return vertex_average(graph, clust)[0]

def build_equivalent_random_graph(graph):
    in_degree = vertex_average(graph, "in")
    out_degree = vertex_average(graph, "out")
    return random_graph(graph.num_vertices(), lambda: (
        np.random.poisson(round(in_degree[0])),
        np.random.poisson(round(out_degree[0]))
        ))
    
def analysis_procedure(agent_files, read_perceptions=False, read_contacts=True):
    print("\nread_perceptions: " + str(read_perceptions)
            + ", read_contacts: " + str(read_contacts))
    g = load_graph(
            arguments.agent_files,
            read_perceptions=read_perceptions,
            read_contacts=read_contacts
            )
    rand_g = build_equivalent_random_graph(g)

    table = PrettyTable()
    table.field_names = ["L_observed", "L_random", "C_observed", "C_random"]
    table.add_row([
        average_shortest_path_length(g),
        average_shortest_path_length(rand_g),
        clustering_coefficient(g),
        clustering_coefficient(rand_g)
        ])
    table.set_style(PLAIN_COLUMNS)
    print(table)

def build_arg_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument(
            'agent_files', metavar='F', type=pathlib.Path, nargs='+',
            help="Agents json file"
            )
    return parser

if __name__ == "__main__":
    arguments = build_arg_parser().parse_args()
    analysis_procedure(arguments.agent_files, read_perceptions=False,
            read_contacts=True)
    analysis_procedure(arguments.agent_files, read_perceptions=True,
            read_contacts=False)
    analysis_procedure(arguments.agent_files, read_perceptions=True,
            read_contacts=True)
