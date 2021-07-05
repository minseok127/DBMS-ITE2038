SELECT t.name
FROM Trainer t, CatchedPokemon cp
WHERE t.id = cp.owner_id
GROUP BY t.name, cp.pid
HAVING COUNT(*) >= 2
ORDER BY t.name