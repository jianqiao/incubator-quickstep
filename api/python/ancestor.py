import time
from quickstep import Quickstep

qs = Quickstep('./quickstep_client')

vertices = [1299454, 31623412, 31739916, 31593563]

# Truncate old tables.
qs.execute('DROP TABLE cur; DROP TABLE next; DROP TABLE answer;')
qs.execute('CREATE TABLE cur(id INT); CREATE TABLE next(id INT); CREATE TABLE answer(id INT);')

# Initialize work table.
init_stmts = ''
for v in vertices:
  init_stmts += 'INSERT INTO cur VALUES(' + str(v) + '); '
qs.execute(init_stmts)
qs.execute('INSERT INTO answer SELECT id FROM cur;')

# Loop body
loop_stmts = ''
loop_stmts += 'DROP TABLE next; CREATE TABLE next(id int); '
loop_stmts += 'INSERT INTO next SELECT edge.src FROM cur, edge ' + \
              'WHERE cur.id = edge.dst GROUP BY edge.src; '
loop_stmts += 'DROP TABLE cur; CREATE TABLE cur(id int); '
loop_stmts += 'INSERT INTO cur SELECT id FROM next WHERE id NOT IN (SELECT id FROM answer); '
loop_stmts += 'INSERT INTO answer SELECT id FROM cur; '

for i in range(50):
  print '--\nIteration ' + str(i) + ':'
  start = time.time()
  qs.execute(loop_stmts)
  end = time.time()

  # We may skip this information for performance.
  num_vertices = qs.execute('COPY SELECT COUNT(*) FROM answer TO stdout;')
  print 'Size of graph: ' + num_vertices.rstrip()
  print 'Time elapsed: ' + str(end - start)

  num_workset = qs.execute('COPY SELECT COUNT(*) FROM cur TO stdout;').rstrip()
  if num_workset == '0':
    break

