import matplotlib
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator
from matplotlib.ticker import FuncFormatter
from pandas import read_csv
from os import path, makedirs
import sys

graph_path = "./data/graph/"

def form1(y, pos):
    """ This function returns a string with 3 decimal places, given the input x"""
    return '%.1f' % y

data = read_csv(sys.argv[1])
x = data['vzdialenost']
y = data['priemer']
top = max(data['priemer'])*1.005
bottom = max(data['priemer'])*0.99

# Plot
plt.plot(x, y)
plt.xlabel('Vzdialenosť (mm)')
plt.ylabel('Priemer (mm)')
# formatter = FuncFormatter(form1)
# plt.gca().yaxis.set_major_formatter(FuncFormatter(formatter))
plt.gca().get_yaxis().get_major_formatter().set_useOffset(False)
plt.gca().xaxis.set_major_locator(MaxNLocator(min_n_ticks=15))
plt.gca().yaxis.set_major_locator(MaxNLocator(min_n_ticks=30))
plt.gca().set_xlim(0)
plt.gca().set_ylim(bottom, top)
plt.grid(linestyle=':', linewidth=1)
plt.gcf().set_size_inches(18, 8)
makedirs(graph_path, exist_ok=True)
plt.subplots_adjust(left=0.055, bottom=0.055, right=0.945, top=0.945, wspace=0, hspace=0)
plt.savefig(graph_path + '%s.png' % path.splitext(path.basename(sys.argv[1]))[0])
sys.stdout.write('OK')
