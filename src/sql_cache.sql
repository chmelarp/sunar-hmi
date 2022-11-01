
-- starter
SELECT t.camera, s."time", s.position[0], s.position[1], s.size[0], s.size[1], t.dataset, t.video FROM ONLY sunar.evaluation_tracks t, sunar.evaluation_states s
WHERE t.dataset=s.dataset AND t.video=s.video AND t.camera=s.camera AND t.track=s.track
      AND s."position" IS NOT NULL
      AND t.track=8
ORDER BY s."time", t.dataset, t.video, t.camera
LIMIT 1;

-- all annotations
SELECT a.camera, a."time", a.position[0], a.position[1], a.size[0], a.size[1] FROM ONLY sunar.annotations a
WHERE a.dataset=2 AND a.video=2
ORDER BY a."time", a.camera

-- videos
SELECT v.camera, v."name" FROM ONLY sunar.videos v
WHERE v.dataset=2 AND v.video=2
ORDER BY v.camera

-- annots
SELECT a.camera, a."time", a.position[0], a.position[1], a.size[0], a.size[1] FROM ONLY sunar.annotations a
WHERE a.dataset=2 AND a.video=2
ORDER BY a."time", a.camera

-- states overlapping annotations
SELECT st.camera, st.time, st.position[0], st.position[1], st.size[0], st.size[1], st.track,  sunar.overlaps(anst.position, anst.size, st.position, st.size)
 FROM ONLY sunar.states st LEFT JOIN ONLY sunar.annotation_states anst
   ON st.dataset=anst.dataset AND st.video=anst.video AND st.camera=anst.camera AND (st.time >= anst.time-2) AND (st.time <= anst.time+2)
 WHERE st.dataset=2 AND st.video=2 AND st.time > (3985-4) AND st.time < (7918+4) -- AND sunar.overlaps(anst.position, anst.size, st.position, st.size) > 0.01
 ORDER BY time, camera