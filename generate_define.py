# Script to generate auxiliary macro for tostr.h

n  = 30
lst = map( lambda v: "a%02d"%(v+1), xrange(n+1) )
lst2 = map( lambda v: "#"+v, lst )
lst_num = list( reversed(map(lambda v:"%02d"%v,xrange(n+2))) )
print "#define TOSTR__CHOOSE_NTH(%s, num, ...) num" % ','.join( lst )
print "#define TOSTR__GET_ARG_NUM(...) TOSTR__CHOOSE_NTH( __VA_ARGS__, %s )" %  ', '.join(lst_num)

for i in xrange(n+1):
    print "#define TOSTR__EXPAND_EACH_%02d(%s) %s" % ( i, ','.join(lst[:i]), ', '.join(lst2[:i] ) )
