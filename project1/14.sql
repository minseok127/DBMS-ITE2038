SELECT name
FROM Pokemon
WHERE type = 'Grass' AND id IN (
  SELECT before_id
  FROM Evolution )