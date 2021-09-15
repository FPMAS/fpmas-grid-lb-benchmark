import matplotlib.pyplot as plt
import argparse
import pathlib
import re
import csv
from operator import itemgetter

class ParamSet():
    def __init__(self, alpha, gamma, beta, mu):
        self.alpha = alpha
        self.gamma = gamma
        self.beta = beta
        self.mu = mu

param_sets = {
        "realistic": ParamSet(30, 10, 15, 20),
        "behavior": ParamSet(100000, 10, 30000, 20),
        "communication": ParamSet(30, 10000, 15, 20000),
        "intense": ParamSet(100000, 10000, 30000, 20000)
        }

def build_arg_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument(
            "input_folder", type=pathlib.Path,
            help="Directory containing LB benchmark output files"
            )
    parser.add_argument('-a', '--alpha', type=int, required=False, default=1,
            help="Agent execution time factor (µs) (default: 1)"
            )
    parser.add_argument('-g', '--gamma', type=int, required=False, default=1,
            help="Agent communication execution time factor (µs) (default: 1)"
            )
    parser.add_argument('-b', '--beta', type=int, required=False, default=1,
            help="Cell execution time factor (µs) (default: 1)"
            )
    parser.add_argument('-u', '--mu', type=int, required=False, default=1,
            help="Cell communication execution time factor (µs) (default: 1)"
            )
    parser.add_argument('-f', '--filters', type=str, required=False, nargs='*',
            help="LB algorithm filter. Files are ignored if they don't match "
            "any filter"
            )
    parser.add_argument('-p', '--param-set', type=str, required=False,
            help="Name of a predefined parameter set (realistic, behavior, "
            "communication, intense)"
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

def build_time_data(files, param_set):
    csv_data = [read_csv(file) for file in files]
    x = []
    y = []
    for j in range(min([len(data) for data in csv_data])):
        x.append(j)
        measures = []
        for i in range(len(files)):
            measures.append([float(value) for value in csv_data[i][j]])
        y.append(time_step_execution_estimation(
            measures, param_set.alpha, param_set.gamma, param_set.beta, param_set.mu
            ))
    return (x, y)

if __name__ == "__main__":
    args = build_arg_parser().parse_args()
    files = build_file_dataset(args.input_folder)

    param_set_str = args.param_set if args.param_set is not None else\
            "alpha=" + str(args.alpha) + ", "\
            "gamma=" + str(args.gamma) + ", "\
            "beta=" + str(args.beta) + ", "\
            "mu=" + str(args.mu)
    plt.title("Load Balancing efficiency (" + param_set_str + ")")
    perf = {}
    param_set = ParamSet(args.alpha, args.gamma, args.beta, args.mu)\
            if args.param_set is None\
            else param_sets[args.param_set]

    for (lb_algorithm, lb_files) in files.items():
        match = False
        if(args.filters is not None):
            lb_filter = 0
            while(lb_filter < len(args.filters) and not match):
                if(re.match(args.filters[lb_filter], lb_algorithm)):
                    match = True
                lb_filter+=1
        else:
            match = True
        
        if(match):
            xy = build_time_data(lb_files, param_set)
            plt.plot(xy[0], xy[1], label=lb_algorithm)
            perf[lb_algorithm] = sum(xy[1])
    rank = 1
    for item in sorted(perf.items(), key=itemgetter(1)):
        print(str(rank) + ": " + item[0] + " (" + str(item[1]) + ")")
        rank += 1

    plt.xlabel("Time step")
    plt.ylabel("Estimated execution time (µs)")

    plt.legend()

    def on_resize(event):
        plt.figure(1).tight_layout()
        plt.figure(1).canvas.draw()

    cid = plt.figure(1).canvas.mpl_connect('resize_event', on_resize)

    plt.show()

