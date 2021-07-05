SELECT t.name, SUM(cp.level) AS 'LEVLE SUM'
FROM CatchedPokemon cp, Trainer t
WHERE cp.owner_id = t.id
GROUP BY t.name
HAVING SUM(cp.level) = (
  SELECT tmp.MAXSUM
  FROM (
    SELECT SUM(cp1.level) AS 'MAXSUM'
    FROM CatchedPokemon cp1, Trainer t1
    WHERE cp1.owner_id = t1.id
    GROUP BY t1.name
    ORDER BY SUM(cp1.level) DESC
    LIMIT 1) AS tmp)
ORDER BY t.name