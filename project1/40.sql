SELECT tmp.name, tmp.nickname
FROM (
  SELECT c.name, cp.level, cp.nickname
  FROM Trainer t, City c, CatchedPokemon cp
  WHERE t.hometown = c.name AND t.id = cp.owner_id ) AS tmp
WHERE tmp.level IN (
  SELECT MAX(cp.level) AS 'level'
  FROM Trainer t, City c, CatchedPokemon cp
  WHERE t.hometown = c.name AND t.id = cp.owner_id AND tmp.name = c.name
  GROUP BY c.name )
ORDER BY tmp.name