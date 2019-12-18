#!/usr/bin/python3

import matplotlib.pyplot as plt
from math import ceil

def plot(x_axis, y_axis, title, x_label, y_label, legend, type='linear'):
    plt.figure(figsize=(12, 6))
    plt.subplot(111)
    plt.xscale(type)
   
    for i in range(0, len(x_axis)):
        plt.plot(x_axis[i], y_axis[i], marker='o')
        
        xint = range(min(x_axis[i]), ceil(max(x_axis[i]))+1)
        plt.xticks(xint)

    plt.suptitle(title)
    plt.xlabel(x_label)
    plt.ylabel(y_label)
    plt.legend(legend)
    plt.draw()


def get_data(file):
    rounds_played = 0
    sim_played = 0
    sim_rounds = 0
    total_time = 0
    avg_time = 0

    f = open(file, 'r')
    for line in f:
        if 'Number of rounds played' in line:
            rounds_played = int(line.split(':')[1])
        elif 'Total number of simulated games' in line:
            sim_played = int(line.split(':')[1])
        elif 'Total number of simulated rounds' in line:
            sim_rounds = int(line.split(':')[1])
        elif 'Total number of simulated steps' in line:
            sim_steps = int(line.split(':')[1])
        elif 'Total time (seconds)' in line:
            total_time = float(line.split(':')[1])
        elif 'Average time for one round (seconds)' in line:
            avg_time = float(line.split(':')[1])
    f.close()

    return (rounds_played, sim_played, sim_rounds, sim_steps, total_time, avg_time)

def get_graphs(tech, num_threads, x_axis, y_axis_perf, y_axis_scal, y_axis_eff):
    tech = tech.lower()
    
    data = []
    serial_data = get_data('./out_' + tech + '_1')

    for tid in num_threads:
        (rounds_played, sim_played, sim_rounds, sim_steps, total_time, avg_time) = get_data('./out_' + tech+ '_' + str(tid))
        data.append((rounds_played, sim_played, sim_rounds, sim_steps, total_time, avg_time))
    
    if tech == 'hybrid':
        x_axis += [[2, 4, 8, 16, 32]]
    else:
        x_axis += [[1, 2, 4, 8, 16]]

    y_axis_perf += [[d[1] for d in data]]
    y_axis_scal += [[d[1] / serial_data[1] for d in data]]

    
    y_axis_eff += [[data[i][1] / serial_data[1] / num_threads[i] for i in range(0, len(data))]]

    return (x_axis, y_axis_perf, y_axis_scal, y_axis_eff)


def main():
    num_threads = [1, 2, 4, 8, 16]
    x_axis = []
    y_axis_perf = []
    y_axis_scal = []
    y_axis_eff = []

    techs = ['omp', 'pthreads', 'mpi', 'hybrid']
    for tech in techs:
        (x_axis, y_axis_perf, y_axis_scal, y_axis_eff) = get_graphs(tech, num_threads, x_axis, y_axis_perf, y_axis_scal, y_axis_eff)

    plot(x_axis, y_axis_perf, 'Performance ', 'Number of threads', 'Number of games simulated', techs)
    plot(x_axis, y_axis_scal, 'Scalability (weak scaling) ', 'Number of threads', 'Speedup', techs)
    plot(x_axis, y_axis_eff, 'Efficiency ', 'Number of threads', 'Efficiency', techs)

    plt.show()


if __name__ == "__main__":
    main()
 