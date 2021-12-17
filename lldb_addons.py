# noinspection PyUnresolvedReferences
import lldb


# print summary of a class with an implemented _debug_string() method:
# DEBUG_STRING_METHOD { return ... }
# after adding such method to a new class, don't forget to mention it in .lldbinit!
def class_with_debug_string(valobj, internal_dict, options):
    # assign the object to v
    if valobj.TypeIsPointerType():
        v = valobj.Dereference()
        if not v.GetAddress().IsValid():
            return "NULL"
    else:
        v = valobj
    # s will be a formatted std::string — "content"
    s = v.GetNonSyntheticValue().EvaluateExpression('_debug_string()').GetSummary()
    # return it with trimmed quotes
    return "" if s is None else str(s)[1:-1]


# print summary of Token class
# we don't use _debug_string() there — we print a enum value instead
# (we can't print a enum value from C++)
def token_printer(valobj, internal_dict, options):
    return str(valobj.GetChildMemberWithName("type_").GetValue())


# print summary of vk::string_view
# we don't use _debug_string() there — it will be replaced with std::string_view somewhen
def vk_string_view_printer(valobj, internal_dict, options):
    data = valobj.GetChildMemberWithName("_data")
    count = valobj.GetChildMemberWithName("_count").GetValueAsUnsigned()
    string = data.GetPointeeData(0, count)          # not null-terminated
    string.Append(lldb.SBData.CreateDataFromInt(0)) # make null-terminated
    error = lldb.SBError()
    string = string.GetString(error, 0)
    return '"' + string + '"' if not error.Fail() else "error"  # + error.GetCString()


# print summary of FunctionPtr, ClassPtr and other Id<*> classes
# just dereference the 'ptr' field and return summary of a child
def data_ptr_printer(valobj, internal_dict, options):
    # GetNonSyntheticValue() is important, as children are overloaded below
    v = valobj.GetNonSyntheticValue().GetChildMemberWithName("ptr").Dereference()
    if not v.GetAddress().IsValid():
        return "NULL"
    desc = v.GetSummary()
    return "" if desc is None else desc


# handle expanding of FunctionPtr, ClassPtr and others
# what we do is vanishing the 'ptr' field on expanding:
# instead of having a hierarchy "item > ptr > name and other props",
# we'll see "item > name and other props", which is very handy
class data_ptr_children(object):
    def __init__(self, valobj, dict):
        self.ptr = valobj.GetChildMemberWithName("ptr")

    def is_notnull(self):
        return self.ptr.Dereference().GetAddress().IsValid()

    def num_children(self, max_count):
        return self.ptr.GetNumChildren() if self.is_notnull() else 0

    def get_child_at_index(self, index):
        return self.ptr.GetChildAtIndex(index)

    def get_child_index(self, name):
        return self.ptr.GetChildIndex(name)

    def has_children(self):
        return self.is_notnull()


# print summary of VertexPtr and VertexAdaptor<*>
# format: "{type} {str_val} ↓{n_children}"
# we don't use _debug_string() there — we want op_name as a enum value
def vertex_printer(valobj, internal_dict, options):
    v = valobj.GetChildMemberWithName("impl_").Dereference()
    if not v.GetAddress().IsValid():
        return "NULL"
    op_name = v.GetChildMemberWithName("type_").GetValue()
    n_children = v.GetChildMemberWithName("n").GetValueAsUnsigned()
    if op_name in ["op_var", "op_func_name", "op_callback_of_builtin", "op_instance_prop", "op_int_const", "op_float_const", "op_func_call"]:
        str_val = " " + str(v.EvaluateExpression('get_string()').GetSummary())
    else:
        str_val = ""
    return op_name + str_val + (' ↓' + str(n_children) if n_children > 0 else ' ∅')


# handle expanding of VertexPtr and VertexAdaptor<*>
# here we insert fake children ith0, ith1 which can also be recursively expanded
# so, a VertexPtr object will have (1+n) nodes in debugger: impl and ith-s
class vertex_children(object):
    def __init__(self, valobj, dict):
        self.impl = valobj.GetChildMemberWithName("impl_")
        # we need the VertexPtr type to cast ith(i) calls to it
        type = self.impl.GetTarget().FindFirstType("VertexPtr")
        self.type_VertexPtr = type if type.IsValid() else None

    def is_notnull(self):
        return self.impl.Dereference().GetAddress().IsValid()

    def num_children(self, max_count):
        if not self.is_notnull():
            return 0
        if self.type_VertexPtr is None:
            return 1
        n = self.impl.GetChildMemberWithName("n").GetValueAsUnsigned()
        return min(max_count, 1 + n)

    def get_child_at_index(self, index):
        if index == 0:
            return self.impl
        if index > 0:
            ith_i = self.impl.Dereference().EvaluateExpression("ith(" + str(index - 1) + ")")
            return self.impl.CreateValueFromData("ith" + str(index - 1), ith_i.GetData(), self.type_VertexPtr)
        return None

    def get_child_index(self, name):
        if name == "impl_":
            return 0
        if name.startswith("ith"):
            return int(name[3:]) + 1
        return None

    def has_children(self):
        return self.is_notnull()
