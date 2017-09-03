import time
from quickstep import Quickstep

qs = Quickstep('./quickstep_client')

src_vertices = [981856, 981899, 981913, 1255431, 1255572, 1255586]
dst_vertices = [1299454, 31623412, 31739916, 31593563]

max_depth = 10

# First generate the backward lineage subgraph within the specified max depth.

print "Prepare tables ..."

# Truncate old tables.
qs.execute('DROP TABLE cur; DROP TABLE next;')
qs.execute('CREATE TABLE cur(id INT); CREATE TABLE next(id INT);')
qs.execute('DROP TABLE answer; CREATE TABLE answer(id INT);')

# Create subgraph edges table.
qs.execute('DROP TABLE sg_conn;')
qs.execute('CREATE TABLE sg_conn(src INT, dst INT, depth INT);')

# Initialize work table.
init_stmts = ''
for v in dst_vertices:
  init_stmts += 'INSERT INTO cur VALUES(' + str(v) + '); '
qs.execute(init_stmts)
qs.execute('INSERT INTO answer SELECT id FROM cur;')

print "Generating backward lineage subgraph ..."

# Loop body.
loop_stmts = ''
loop_stmts += 'INSERT INTO sg_conn SELECT e.src, e.dst, $depth ' + \
              'FROM cur, conn e WHERE cur.id = e.dst; '
loop_stmts += 'DROP TABLE next; CREATE TABLE next(id int); '
loop_stmts += 'INSERT INTO next SELECT e.src FROM cur, conn e ' + \
              'WHERE cur.id = e.dst GROUP BY e.src; '
loop_stmts += 'DROP TABLE cur; CREATE TABLE cur(id int); '
loop_stmts += 'INSERT INTO cur SELECT id FROM next WHERE id NOT IN (SELECT id FROM answer); '
loop_stmts += 'INSERT INTO answer SELECT id FROM cur;'

for i in range(max_depth):
  print '--\nIteration ' + str(i + 1) + ':'
  start = time.time()
  qs.execute(loop_stmts.replace('$depth', str(i+1)))
  end = time.time()

  # We may skip this information for performance.
  num_vertices = qs.execute('COPY SELECT COUNT(*) FROM answer TO stdout;')
  print 'Size of graph: ' + num_vertices.rstrip()
  print 'Time elapsed: ' + str(end - start)

  num_workset = qs.execute('COPY SELECT COUNT(*) FROM cur TO stdout;').rstrip()
  if num_workset == '0':
    break

# Then generate forward lineage sub-subgraph given sg_conn.

print "Analyzing sg_conn table ..."
qs.execute('\\analyze sg_conn\n')

# Truncate old tables.
qs.execute('DROP TABLE cur; DROP TABLE next;')
qs.execute('CREATE TABLE cur(id INT); CREATE TABLE next(id INT);')

# Initialize work table.
init_stmts = ''
for v in src_vertices:
  init_stmts += 'INSERT INTO next VALUES(' + str(v) + '); '
qs.execute(init_stmts)
qs.execute('INSERT INTO cur SELECT id FROM answer WHERE id IN (SELECT id FROM next);')

qs.execute('DROP TABLE answer; CREATE TABLE answer(id INT);')
qs.execute('INSERT INTO answer SELECT id FROM cur;')

print "Generating forward lineage sub-subgraph ..."

# Loop body.
loop_stmts = ''
loop_stmts += 'DROP TABLE next; CREATE TABLE next(id int); '
loop_stmts += 'INSERT INTO next SELECT e.dst FROM cur, sg_conn e ' + \
              'WHERE cur.id = e.src ' + \
              'AND e.depth + $depth <= ' + str(max_depth) + ' GROUP BY e.dst; '
loop_stmts += 'DROP TABLE cur; CREATE TABLE cur(id int); '
loop_stmts += 'INSERT INTO cur SELECT id FROM next WHERE id NOT IN (SELECT id FROM answer); '
loop_stmts += 'INSERT INTO answer SELECT id FROM cur;'

for i in range(max_depth):
  print '--\nIteration ' + str(i + 1) + ':'
  start = time.time()
  qs.execute(loop_stmts.replace('$depth', str(i)))
  end = time.time()

  # We may skip this information for performance.
  num_vertices = qs.execute('COPY SELECT COUNT(*) FROM answer TO stdout;')
  print 'Size of graph: ' + num_vertices.rstrip()
  print 'Time elapsed: ' + str(end - start)

  num_workset = qs.execute('COPY SELECT COUNT(*) FROM cur TO stdout;').rstrip()
  if num_workset == '0':
    break

print "Done."

