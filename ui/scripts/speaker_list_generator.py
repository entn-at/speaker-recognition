import os

data = os.listdir('../../neuralnetwork/data/test')
data = list(map(lambda x: x[:-3], data))

audio = os.listdir('../audio/')
audioA = list(map(lambda x: x[:-4]+'_A', audio))
audioB = list(map(lambda x: x[:-4]+'_B', audio))

d = {}
for f in data:
    if f in audioA or f in audioB:
        d[f] = ''

for line in open('../../neuralnetwork/data/train_condition_5.ndx','r'):
    id,file = line.split()
    if file in d:
        d[file] = id

for line in open('../../neuralnetwork/data/targets_condition5_new.ndx','r'):
    id,file = line.split()
    if file in d:
        d[file] = id

remove = [k for k,v in d.items() if v == '']
for k in remove:
    del d[k]

print(len(d))

spk_dict = {}
for k,v in d.items():
    if v in spk_dict:
        spk_dict[v].append(k)
    else:
        spk_dict[v] = [k]

l1 = []
l2 = []
l3 = []
l4 = []
for k,v in spk_dict.items():
    if len(v) >= 4:
        l4.append(v)
    elif len(v) == 3:
        l3.append(v)
    elif len(v) == 3:
        l3.append(v)
    elif len(v) == 2:
        l2.append(v)
    elif len(v) == 1:
        l1.append(v)

writer = open('./speaker-audio-list.dat','w')
for f in (l4+l3+l2+l1):
    writer.write(' '.join(f) + '\n')
writer.close()
