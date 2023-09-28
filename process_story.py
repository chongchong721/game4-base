import numpy as np
import sys

def int_to_bytes(num,bytelength):
    num = int(num)
    ret = num.to_bytes(bytelength,sys.byteorder)
    return ret
def string_to_bytes(mystr):
    str_bytes = bytearray(mystr,encoding='ascii')
    return str_bytes

#state-transition
class string_state_pair:
    state_name = None

    # state_number corresponding to state_name need to update afterwards
    state_num = 0

    def __init__(self,tostate) -> None:
            self.state_name = tostate

# This only works when the main story string is in one line... Ugly
# Can not support if...

# Is there a way to express (set:$role to "thief")?? no time to think
            
class story_state:
    state_number = 0

    state_name = "default"
    story_string = None
    to_state = None

    def __init__(self,state,n) -> None:
        self.state_number = n
        self.to_state = []
        statename = state[0]
        statename = statename[3:]
        statename = statename.split("{")
        statename = statename[0]
        idx = statename.rfind(' ')
        statename = statename[:idx]
        self.state_name = statename
        self.story_string = state[1]

        for i in range(len(state)-2):
            tostate = state[i+2]
            tostate = tostate[2:-2]
            self.to_state.append(string_state_pair(tostate))




def write_data(all_story_state):
    nstate = len(all_story_state)


    
    # For each state, we have metadata
    # state NO: 4 byte
    # state name length 4 byte
    # state name offset 4 byte
    # story string length 4 byte
    # story string offet 4 byte
    # how many to_state: 4 byte
    # to_state offset: 4 byte

    string_statename = ""
    string_mainstory = ""
    to_state_array = []

    # write a binary string contain all state name
    # write a binary string contain all main story
    # write a binary number array contain all to_state_number

    state_whole_length = 0
    story_whole_length = 0
    to_state_whole_length = 0


    with open("metadata","wb") as f:
        magic = "stmd"
        size = (nstate * 28)

        f.write(bytearray(magic,encoding="ascii"))
        f.write(size.to_bytes(4,sys.byteorder))

        metadatabyte = bytes()
        # offsets are all in byte
        state_name_offset = 0
        story_offset = 0
        to_state_offset = 0

        for i in range(len(all_story_state)):
            state = all_story_state[i]
            metadatabyte += int_to_bytes(state.state_number,4)

            length_state_name = len(state.state_name)
            state_whole_length += length_state_name
            metadatabyte += int_to_bytes(length_state_name,4)
            metadatabyte += int_to_bytes(state_name_offset,4)
            state_name_offset += length_state_name

            length_story = len(state.story_string)
            story_whole_length += length_story
            metadatabyte += int_to_bytes(length_story,4)
            metadatabyte += int_to_bytes(story_offset,4)
            story_offset += length_story

            length_to_state = len(state.to_state)
            to_state_whole_length += length_to_state
            metadatabyte += int_to_bytes(length_to_state,4)
            metadatabyte += int_to_bytes(to_state_offset,4)
            to_state_offset += length_to_state

        f.write(metadatabyte)
        print(len(metadatabyte))

    with open("statename","wb") as f:
        magic = "stat"
        size = state_whole_length 
        f.write(bytearray(magic,encoding="ascii"))
        f.write(size.to_bytes(4,sys.byteorder))   
        for i in range(len(all_story_state)):
            state = all_story_state[i]
            strbytes = string_to_bytes(state.state_name)
            f.write(strbytes)

    with open("story","wb") as f:
        magic = "stor"
        size = story_whole_length
        f.write(bytearray(magic,encoding="ascii"))
        f.write(size.to_bytes(4,sys.byteorder))   
        for i in range(len(all_story_state)):
            state = all_story_state[i]
            strbytes = string_to_bytes(state.story_string)
            f.write(strbytes)


    with open("tostate","wb") as f:
        magic = "to00"
        size = to_state_whole_length * 4
        f.write(bytearray(magic,encoding="ascii"))
        f.write(size.to_bytes(4,sys.byteorder))   
        for i in range(len(all_story_state)):
            state = all_story_state[i]
            for j in range(len(state.to_state)):
                intbytes = int_to_bytes(state.to_state[j].state_num,4)
                f.write(intbytes)







if __name__ == "__main__":
    with open("./jailbreak.twee","r") as f:
        lines = f.readlines()

        states = []
        current_state = []
        for line in lines:

            if line[0] == ":" and line[1] == ":":
                if len(current_state ) > 0:
                    new_state = []
                    for i in range(len(current_state)):
                        new_state.append(current_state[i])
                    states.append(new_state)
                    current_state.clear()
                    current_state.append(line[:-1])
                else:
                    current_state.append(line[:-1])
            else:
                if line.isspace():
                    continue
                else:
                    current_state.append(line[:-1])

        states.append(current_state)

        all_story_state = []

        states = states[2:]

        i = 0

        for state in states:
            all_story_state.append(story_state(state,i))
            i += 1


        for state in all_story_state:
            for to_state in state.to_state:
                for test_state in all_story_state:
                    if to_state.state_name == test_state.state_name:
                        to_state.state_num = test_state.state_number
                        break
                    else:
                        continue

        print(states)

        write_data(all_story_state)







