SELECT t.hometown, AVG(cp.level) AS 'AVG Level'
FROM Trainer t, CatchedPokemon cp
WHERE t.id = cp.owner_id
GROUP BY t.hometown
ORDER BY AVG(cp.level)