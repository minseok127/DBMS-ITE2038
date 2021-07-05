SELECT t.name, MAX(cp.level) AS 'MAX LEVEL'
FROM Trainer t, CatchedPokemon cp
WHERE t.id = cp.owner_id
GROUP BY cp.owner_id
HAVING COUNT(*) >= 4
ORDER BY t.name