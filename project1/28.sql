SELECT t.name, AVG(cp.level) AS 'AVG LEVEL'
FROM Trainer t, CatchedPokemon cp, Pokemon p
WHERE t.id = cp.owner_id AND cp.pid = p.id AND (p.type = 'Electric' OR p.type = 'Normal')
GROUP BY t.name
ORDER BY AVG(cp.level)