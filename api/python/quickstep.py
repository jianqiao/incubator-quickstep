from subprocess import Popen, PIPE, STDOUT

class Quickstep(object):
  def __init__(self, client_path, server_ip = None, server_port = None):
    self.client_path = client_path
    self.server_ip = server_ip
    self.server_port = server_port

  def __addSemicolon__(self, query):
    return query + ('' if query.endswith(';') else ';')

  def execute(self, query):
    # query = self.__addSemicolon__(query)

    cmd = self.client_path
    if self.server_ip is not None:
      cmd += ' -cli_network_ip=' + self.server_ip
    if self.server_port is not None:
      cmd += ' -cli_network_port=' + self.server_port

    client_process = Popen([cmd], stdout=PIPE, stdin=PIPE, stderr=PIPE)
    out, err = client_process.communicate(input = query)

    # Currently we ignore the stderr stream and just return stdout's data.
    return out
