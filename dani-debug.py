from src.RaspberryPi.SharedMemory import SharedMemory

if __name__ =="__main__":
    eye_tracking_memory = SharedMemory("eye_tracking", size=10, create=False)


    print("[0] - looking away\n[1] - looking at")
    while True:
        choice = input("Make selection: ")

        if choice == '0':
            eye_tracking_memory.write_string("[0]")
        elif choice == '[1]':
            pass