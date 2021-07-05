SELECT DISTINCT tbl1.name
FROM (
  SELECT p.name
  FROM Trainer t, CatchedPokemon cp, Pokemon p
  WHERE t.id = cp.owner_id AND cp.pid = p.id AND t.hometown = 'Sangnok City') AS tbl1,
  (
    SELECT p1.name
    FROM Trainer t1, CatchedPokemon cp1, Pokemon p1
    WHERE t1.id = cp1.owner_id AND cp1.pid = p1.id AND t1.hometown = 'Brown City') AS tbl2
WHERE tbl1.name = tbl2.name
ORDER BY tbl1.name