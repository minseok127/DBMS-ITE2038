SELECT COUNT(*) AS 'COUNT'
FROM (
  SELECT DISTINCT p.id
  FROM Trainer t, CatchedPokemon cp, Pokemon p
  WHERE t.id = cp.owner_id AND cp.pid = p.id AND t.hometown = 'Sangnok City') AS Ptype