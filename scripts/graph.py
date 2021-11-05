from tkinter import *
from tkinter import filedialog
import matplotlib.pyplot as plt
from pandas import read_csv
from sys import exit

filename = ""

# Function for opening the
# file explorer window
def browseFiles():
    global filename
    filename = filedialog.askopenfilename(initialdir = ".",
                                          title = "Select a File",
                                          filetypes = (("CSV files",
                                                        "*.csv*"),
                                                       ("all files",
                                                        "*.*")))
    # Change label contents
    if type(filename) is not tuple:
        label_file_explorer.configure(text=filename)
    else:
        filename = ""

def drawGraph():
    data = read_csv(filename)
    # Plot
    x = range(len(data['vzdialenost']))
    plt.plot(x, data['priemer'])
    plt.xlabel('Vzdialenosť (mm)')
    plt.ylabel('Priemer (mm)')
    # setting the xticks to have 3 decimal places
    # yy, locs = plt.yticks()
    # ll = ['%.4f' % a for a in yy]
    # plt.yticks(yy, ll)
    plt.show()


window = Tk()
window.title('CSV graf')
# img = PhotoImage(file='graph.png')
# window.tk.call('wm', 'iconphoto', window._w, img)
window.geometry("400x230")
label_file_explorer = Label(window,
                            text = "Vyberte CSV súbor s dátami",
                            width = 50, height = 4,
                            fg = "blue")
button_explore = Button(window,
                        text = "Prehľadávať",
                        command = browseFiles)
button_graph = Button(window,
                        text = "Vykresli graf",
                        command = drawGraph)
button_exit = Button(window,
                     text = "Ukonči",
                     command = exit)

label_file_explorer.grid(column = 1, row = 1)
button_explore.grid(column = 1, row = 2)
Label(window).grid(column=1, row = 3)
button_graph.grid(column = 1, row = 4)
Label(window).grid(column=1, row = 5)
button_exit.grid(column = 1, row = 6)

window.mainloop()
