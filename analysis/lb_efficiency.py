import matplotlib.pyplot as plt
import argparse
import pathlib
import re
import csv
from operator import itemgetter


def build_arg_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument(
            "input_folder", type=pathlib.Path,
            help="Directory containing LB benchmark output files"
            )
    parser.add_argument('-a', '--alpha', type=int, required=False, default=1,
            help="Agent execution time factor (default: 1)"
            )
    parser.add_argument('-g', '--gamma', type=int, required=False, default=1,
            help="Agent communication execution time factor (default: 1)"
            )
    parser.add_argument('-b', '--beta', type=int, required=False, default=1,
            help="Cell execution time factor (default: 1)"
            )
    parser.add_argument('-u', '--mu', type=int, required=False, default=1,
            help="Cell communication execution time factor (default: 1)"
            )


    return parser

def time_step_execution_estimation(measures, alpha, gamma, beta, mu):
    t_dist = max([measure[1] + measure[2] for measure in measures])
    t_agent = max([alpha * measure[3] + gamma * measure[5] for measure in
            measures])
    t_cell = max([beta * measure[4] + mu * measure[6] for measure in
            measures])
    return t_dist + t_agent + t_cell

def build_file_dataset(input_folders):
    files = {}
    for file in input_folders.iterdir():
        match = re.match(r'(.*?)\.\d+\.csv', file.name)
        if match is not None:
            if match[1] in files:
                files[match[1]].append(file)
            else:
                files[match[1]] = [file]
    return files

def read_csv(file):
    with open(file) as csvfile:
        reader = csv.reader(csvfile)
        return list(reader)[1:]

def build_time_data(files, alpha, gamma, beta, mu):
    csv_data = [read_csv(file) for file in files]
    x = []
    y = []
    for j in range(min([len(data) for data in csv_data])):
        x.append(j)
        measures = []
        for i in range(len(files)):
            measures.append([float(value) for value in csv_data[i][j]])
        y.append(time_step_execution_estimation(measures, alpha, gamma, beta, mu))
    return (x, y)

if __name__ == "__main__":
    args = build_arg_parser().parse_args()
    files = build_file_dataset(args.input_folder)
    print(files)
    plt.title("Load Balancing efficiency")
    perf = {}
    for (lb_algorithm, lb_files) in files.items():
        xy = build_time_data(lb_files, args.alpha, args.gamma, args.beta,
                args.mu)
        plt.plot(xy[0], xy[1], label=lb_algorithm)
        perf[lb_algorithm] = sum(xy[1])
    rank = 1
    for item in sorted(perf.items(), key=itemgetter(1)):
        print(str(rank) + ": " + item[0] + " (" + str(item[1]) + ")")
        rank += 1


    plt.legend()
    plt.show()

