#!/usr/bin/ruby

# todo: rewrite this in aqua! assuming it ever gets to the point.

DEF_CXX    = %w(g++ -std=c++17 -O3)
DEF_WFLAGS = %w(-Wall -Wextra)
DEF_CFLAGS = %w(-g3 -ggdb3)
DEF_LFLAGS = %w()


EXEFILES = [
  "mksum",
]

SOURCEDIRS = [
  "src",
]
INCDIRS = [
  "include",
  "src",
]

CXXEXTPATTERN = /\.(cpp|cxx|c\+\+|cc|c)$/i

def getglobs(rx, *idirs)
  dirs = []
  idirs.each{|d| dirs |= Dir.glob(d) }
  dirs.each do |d|
    Dir.glob(d +"/*") do |itm|
      next unless (File.file?(itm) && itm.match?(rx))
      yield itm
    end
  end
end

def rule(out, name, **desc)
  out.printf("rule %s\n", name)
  desc.each do |name, val|
    val = (
      if val.is_a?(Array) then
        val.join(" ")
      else
        val
      end
    )
    out.printf("  %s = %s\n", name.to_s, val.to_s)
  end
  out.printf("\n")
end

def print_buildrule(out, srcfile)
  objfile = srcfile.gsub(/\.cpp$/, ".o")
  out.printf("build %s: cc %s\n", objfile, srcfile)
  out.printf("  depfile = %s.d\n", srcfile)
  return objfile
end

begin
  sharedobjects = []
  cflags = DEF_CFLAGS.dup
  lflags = DEF_LFLAGS.dup
  wflags = DEF_WFLAGS.dup
  if DEF_CXX.include?("g++") then
    lflags.push("-lstdc++fs")
  end
  INCDIRS.each do |di|
    cflags.push("-I", di)
  end
  File.open("build.ninja", "wb") do |out|
    rule(out, "cc",
      deps: 'gcc',
      depfile: '$in.d',
      command: [*DEF_CXX, *wflags, '-MMD', '-MF', '$in.d', *cflags, '-c', '$in', '-o', '$out'],
      description: '[CC] $in -> $out',
    )
    rule(out, "link",
      command: [*DEF_CXX, *cflags, "-o", '$out', '$in', *lflags],
      description: '[LINK] $out',
    )
    getglobs(/\.cpp$/, "src") do |srcfile|
      sharedobjects.push(print_buildrule(out, srcfile))
    end
    #EXESOURCEFILES.each do |srcfile|
    getglobs(/\.cpp$/, "src/progs") do |srcfile| 
      exe = File.basename(srcfile).gsub(/\.cpp$/, "")
      ofile = print_buildrule(out, srcfile)
      out.printf("build bin/%s: link %s\n", exe, [ofile, sharedobjects].join(" "))
    end
  end
end


