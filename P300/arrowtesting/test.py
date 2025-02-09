#from backend import data
def iotest():
    file_path = "C:/Users/thepi/Documents/Capstone/neuromove/P300/arrowtesting/test1.txt"
    timeID = [123, "asdf"]
    with open(file_path, "a") as f:  # Open in append mode
        f.write(str(timeID) + '\n')
    print("file written to test.txt")

if __name__ == '__main__':
    iotest()