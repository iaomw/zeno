import json

from . import core


def evaluateExpr(expr, frameid):
    frame = frameid
    return eval('f' + repr(expr))

def runScene(graphs, nframes, iopath):
    core.setIOPath(iopath)

    subgkeys = set(graphs.keys())
    for name, nodes in graphs.items():
        core.switchGraph(name)
        loadGraph(nodes, subgkeys)

    applies = []
    nodes = graphs['main']
    for ident, data in nodes.items():

        if 'OUT' in data['options']:
            applies.append(ident)
            datas.append(data)

    core.switchGraph('main')

    for frameid in range(nframes):
        print('FRAME:', frameid)
        for nodes in graphs.values():
            for ident, data in nodes.items():
                name = data['name']
                inputs = data['inputs']
                params = data['params']
                for name, value in params.items():
                    if type(value) is str:
                        value = evaluateExpr(value, frameid)
                        core.setNodeParam(ident, name, value)

        core.frameBegin()
        while core.substepBegin():
            core.applyNodes(applies)
            core.substepEnd()
        core.frameEnd()

    print('EXITING')


def loadGraph(nodes, subgkeys):
    core.clearNodes()

    for ident in nodes:
        data = nodes[ident]
        name = data['name']
        inputs = data['inputs']
        params = data['params']
        options = data['options']

        if name in subgkeys:
            params['name'] = name
            name = 'Subgraph'
        elif name == 'ExecutionOutput':
            name = 'Route'
        core.addNode(name, ident)

        for name, input in inputs.items():
            if input is None:
                continue
            srcIdent, srcSockName = input
            core.bindNodeInput(ident, name, srcIdent, srcSockName)

        for name, value in params.items():
            core.setNodeParam(ident, name, value)

        core.setNodeOptions(ident, set(options))

        core.completeNode(ident)


def dumpDescriptors():
    return core.dumpDescriptors()


__all__ = [
    'runScene',
    'dumpDescriptors',
]
