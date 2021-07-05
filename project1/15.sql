SELECT tmp.owner_id, COUNT
FROM (
  SELECT owner_id, COUNT(*) AS 'COUNT'
  FROM CatchedPokemon
  GROUP BY owner_id
  ORDER BY COUNT(*)) AS tmp
WHERE tmp.COUNT = (
  SELECT MAX(tmp2.COUNT)
  FROM (
    SELECT owner_id, COUNT(*) AS 'COUNT'
    FROM CatchedPokemon
    GROUP BY owner_id
    ORDER BY COUNT(*)) AS tmp2)
ORDER BY tmp.owner_id