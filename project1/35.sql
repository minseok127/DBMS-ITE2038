SELECT p.name
FROM Pokemon p, Evolution e
WHERE p.id = e.before_id AND e.before_id > e.after_id
ORDER BY p.name