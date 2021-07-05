SELECT p.name, cp.level, cp.nickname
FROM Gym g, CatchedPokemon cp, Pokemon p
WHERE g.leader_id = cp.owner_id AND cp.pid = p.id AND cp.nickname LIKE 'A%'
ORDER BY p.name DESC