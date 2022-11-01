#!/bin/bash

select dataset, video, experiment, count(distinct camera), count(distinct track), count(*) from sunar.processed
where experiment>0 and object>0
group by dataset, video, experiment
order by count(distinct camera) desc, count(*) desc, count(track) desc, dataset, video

4;6;1281;3;3;526
7;1;1294;2;2;3434
6;4;1288;2;4;1012
10;2;1311;2;2;786
3;5;1270;2;3;615
6;6;1290;2;2;557
3;9;1274;2;4;412
12;1;1319;2;2;356
12;7;1325;2;2;339
3;3;1268;2;2;315
2;8;1263;2;3;285
2;1;1256;2;2;177



#3
#cp /mnt/data/projects/sunar/sunar-hmi/avss_mcspt_d09_1274.avi ./
#cp /mnt/data/projects/sunar/sunar-hmi/avss_mcspt_d09_1279.avi ./
#cp /mnt/data/projects/sunar/sunar-hmi/avss_mcspt_d09_1265.avi ./

#3
#dist/Release/GNU-Linux-x86/sunar-hmi -a exps -t 1281

#2
dist/Release/GNU-Linux-x86/sunar-hmi -a exps -t 1288 ### :)
dist/Release/GNU-Linux-x86/sunar-hmi -a exps -t 1274
dist/Release/GNU-Linux-x86/sunar-hmi -a exps -t 1263
dist/Release/GNU-Linux-x86/sunar-hmi -a exps -t 1270
dist/Release/GNU-Linux-x86/sunar-hmi -a exps -t 1256
dist/Release/GNU-Linux-x86/sunar-hmi -a exps -t 1268
dist/Release/GNU-Linux-x86/sunar-hmi -a exps -t 1290
dist/Release/GNU-Linux-x86/sunar-hmi -a exps -t 1294 ### :)
dist/Release/GNU-Linux-x86/sunar-hmi -a exps -t 1311
dist/Release/GNU-Linux-x86/sunar-hmi -a exps -t 1319
dist/Release/GNU-Linux-x86/sunar-hmi -a exps -t 1325

