from graph_tool import *
from graph_tool.clustering import *
from graph_tool.stats import *
from graph_tool.topology import *
from pathlib import Path
import sys
import csv

def average_shortest_path_length(graph):
    (counts, bins) = distance_histogram(graph)

    average = 0
    num_vertices = 0
    for i in range(0, len(counts)):
        average += counts[i] * bins[i]
        num_vertices += counts[i]
    return average / num_vertices

def clustering(graph):
    local_c = local_clustering(graph, undirected=False)
    c = 0
    for v in graph.vertices():
        c+=local_c[v]
    return c / graph.num_vertices();

def not_connected_nodes(graph):
    connected = label_largest_component(graph)
    i = 0
    for v in graph.vertices():
        if connected[v] == False:
            i+=1
    return i

if __name__ == "__main__":
    with open("graph_stats.csv", 'w') as csv_output:
        csv_writer = csv.writer(csv_output)
        csv_writer.writerow(["Environment", "C", "L", "Connectivity"])
        for i in range(1, len(sys.argv)):
            g = load_graph(sys.argv[i])
            C =  str(clustering(g))
            L =  str(average_shortest_path_length(g))
            u = not_connected_nodes(g)
            print(sys.argv[i])
            print("C: " + C)
            print("L: " + L)
            print(
                    "Connectivity: " + str(100*(1-u/g.num_vertices()))
                    + "% (u=" + str(u) + ")")
            print()
            csv_writer.writerow([sys.argv[i], C, L, u])


