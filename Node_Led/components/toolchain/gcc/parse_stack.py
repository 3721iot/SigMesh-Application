import os

f_out = open('stack.txt', 'w')

for filepath,dirnames,filenames in os.walk('.'):
    for filename in filenames:
        if os.path.splitext(filename)[-1] == '.su':
            f_in = open(filename, 'r')
            f_out.write(filename+"\n")
            for line in f_in.readlines():
                array = line.split()
                if int(array[1], 10) > 100:
                    f_out.write("\t")
                    f_out.write(array[0].split(':')[3])
                    f_out.write("\t\t\t\t"+array[1]+'\n')                
            f_in.close()
            
f_out.close()
