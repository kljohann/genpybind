def get_user_mro(cls):
    mro = cls.mro()
    assert mro[-2].__class__.__name__ == "pybind11_type"
    return mro[:-2]
