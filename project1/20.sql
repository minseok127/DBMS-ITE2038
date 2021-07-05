SELECT t.name, COUNT(*) AS 'COUNT'
FROM Trainer t, CatchedPokemon cp
WHERE t.id = cp.owner_id AND t.hometown = 'Sangnok City'
GROUP BY t.name
ORDER BY COUNT