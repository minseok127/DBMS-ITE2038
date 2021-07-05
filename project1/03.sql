SELECT AVG(cp.level) AS 'Average Level'
FROM Trainer t, CatchedPokemon cp
JOIN Pokemon AS p ON cp.pid = p.id
WHERE t.id = cp.owner_id AND t.hometown = 'Sangnok City' AND p.type = 'Electric'