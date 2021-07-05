SELECT COUNT(*) AS 'Leader COUNT'
FROM (
  SELECT DISTINCT p.type
  FROM Gym g, CatchedPokemon cp, Pokemon p
  WHERE g.leader_id = cp.owner_id AND cp.pid = p.id AND g.city = 'Sangnok City') AS leader_tbl