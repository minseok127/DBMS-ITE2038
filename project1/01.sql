SELECT t.name
FROM CatchedPokemon cp, Trainer t
WHERE cp.owner_id = t.id
GROUP BY owner_id
HAVING COUNT(*) >= 3
ORDER BY COUNT(*) DESC