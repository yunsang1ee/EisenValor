from __future__ import annotations

import argparse
import struct
import sys
import tempfile
import tkinter as tk
from dataclasses import dataclass
from io import BytesIO
from pathlib import Path
from tkinter import filedialog, messagebox, ttk

import numpy as np
from PIL import Image, ImageTk


HEADER_SIZE = 64
CHUNK_ENTRY_SIZE = 32
CHANNEL_SOURCES = ("R", "G", "B", "A", "0", "1")
VIEW_MODES = ("Composite", "R", "G", "B", "A")
USAGE_NAMES = {
    0: "Albedo",
    1: "Normal",
    2: "ORM",
    3: "Emissive",
    4: "UI",
    5: "Other",
}


@dataclass
class EvtexImage:
    path: Path
    signature: str
    version: int
    guid_hex: str
    is_srgb: bool
    usage: int
    decoded_mode: str
    rgba: np.ndarray

    @property
    def width(self) -> int:
        return int(self.rgba.shape[1])

    @property
    def height(self) -> int:
        return int(self.rgba.shape[0])


def parse_evtex(path: Path) -> EvtexImage:
    blob = path.read_bytes()
    if len(blob) < HEADER_SIZE:
        raise ValueError(f"File too small for EVTX header: {path}")

    signature, version, header_size, _flags = struct.unpack_from("<4sIII", blob, 0)
    signature_text = signature.decode("ascii", errors="replace")
    if signature_text != "EVTX":
        raise ValueError(f"Unexpected signature '{signature_text}' in {path}")

    if header_size < HEADER_SIZE or header_size > len(blob):
        raise ValueError(f"Invalid header size {header_size} in {path}")

    guid_bytes = struct.unpack_from("<16s", blob, 16)[0]
    file_size, chunk_count = struct.unpack_from("<QI", blob, 32)
    if file_size != len(blob):
        raise ValueError(f"File size mismatch in {path}: header={file_size}, actual={len(blob)}")

    table_end = header_size + chunk_count * CHUNK_ENTRY_SIZE
    if table_end > len(blob):
        raise ValueError(f"Chunk table exceeds file size in {path}")

    chunks: dict[str, tuple[int, int, int]] = {}
    for index in range(chunk_count):
        entry_offset = header_size + index * CHUNK_ENTRY_SIZE
        chunk_type, chunk_version, payload_offset, payload_size, _uncompressed_size = struct.unpack_from(
            "<4sIQQQ", blob, entry_offset
        )
        chunk_name = chunk_type.decode("ascii", errors="replace")
        if payload_offset + payload_size > len(blob):
            raise ValueError(f"Chunk '{chunk_name}' exceeds file size in {path}")
        chunks[chunk_name] = (chunk_version, int(payload_offset), int(payload_size))

    is_srgb = False
    usage = 5
    if "META" in chunks:
        _meta_version, meta_offset, meta_size = chunks["META"]
        if meta_size >= 8:
            meta_is_srgb, meta_usage = struct.unpack_from("<II", blob, meta_offset)
            is_srgb = meta_is_srgb != 0
            usage = int(meta_usage)

    if "DATA" not in chunks:
        raise ValueError(f"DATA chunk not found in {path}")

    _data_version, data_offset, data_size = chunks["DATA"]
    dds_bytes = blob[data_offset : data_offset + data_size]
    if dds_bytes[:4] != b"DDS ":
        raise ValueError(f"DATA chunk does not start with DDS magic in {path}")

    with Image.open(BytesIO(dds_bytes)) as image:
        decoded_mode = image.mode
        rgba_image = image.convert("RGBA")
        rgba = np.asarray(rgba_image, dtype=np.uint8).copy()

    return EvtexImage(
        path=path,
        signature=signature_text,
        version=version,
        guid_hex=guid_bytes.hex(),
        is_srgb=is_srgb,
        usage=usage,
        decoded_mode=decoded_mode,
        rgba=rgba,
    )


def channel_index(source: str) -> int | None:
    if source == "R":
        return 0
    if source == "G":
        return 1
    if source == "B":
        return 2
    if source == "A":
        return 3
    return None


def remap_channels(rgba: np.ndarray, mapping: tuple[str, str, str, str]) -> np.ndarray:
    output = np.empty_like(rgba)
    for dst_index, source in enumerate(mapping):
        src_index = channel_index(source)
        if src_index is None:
            output[:, :, dst_index] = 0 if source == "0" else 255
        else:
            output[:, :, dst_index] = rgba[:, :, src_index]
    return output


def extract_channel_preview(rgba: np.ndarray, channel: str) -> Image.Image:
    if channel == "Composite":
        return composite_on_checker(Image.fromarray(rgba, mode="RGBA"))

    src_index = channel_index(channel)
    if src_index is None:
        raise ValueError(f"Invalid channel view mode: {channel}")

    gray = rgba[:, :, src_index]
    rgb = np.repeat(gray[:, :, None], 3, axis=2)
    return Image.fromarray(rgb, mode="RGB")


def composite_on_checker(image: Image.Image, tile_size: int = 16) -> Image.Image:
    rgba = image.convert("RGBA")
    width, height = rgba.size

    yy, xx = np.indices((height, width))
    checker = ((xx // tile_size + yy // tile_size) % 2).astype(np.uint8)
    bg = np.where(checker[:, :, None] == 0, 48, 96).astype(np.uint8)
    bg_rgba = np.dstack((bg, bg, bg, np.full((height, width), 255, dtype=np.uint8)))

    background = Image.fromarray(bg_rgba, mode="RGBA")
    return Image.alpha_composite(background, rgba).convert("RGB")


def resize_to_fit(image: Image.Image, max_size: tuple[int, int]) -> Image.Image:
    width, height = image.size
    max_width, max_height = max_size
    scale = min(max_width / max(1, width), max_height / max(1, height), 1.0)
    target_size = (max(1, int(width * scale)), max(1, int(height * scale)))
    if target_size == image.size:
        return image
    return image.resize(target_size, Image.Resampling.LANCZOS)


class EvtexViewerApp:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title("EVTEX Viewer")
        self.root.geometry("1280x900")

        self.evtex: EvtexImage | None = None
        self.main_photo: ImageTk.PhotoImage | None = None
        self.channel_photos: dict[str, ImageTk.PhotoImage] = {}

        self.view_mode_var = tk.StringVar(value="Composite")
        self.map_vars = {
            "R": tk.StringVar(value="R"),
            "G": tk.StringVar(value="G"),
            "B": tk.StringVar(value="B"),
            "A": tk.StringVar(value="A"),
        }
        self.status_var = tk.StringVar(value="Open an .evtex file to inspect channels.")

        self._build_ui()

    def _build_ui(self) -> None:
        toolbar = ttk.Frame(self.root, padding=8)
        toolbar.pack(side=tk.TOP, fill=tk.X)

        ttk.Button(toolbar, text="Open .evtex", command=self.open_file_dialog).pack(side=tk.LEFT)
        ttk.Button(toolbar, text="Save Remapped PNG", command=self.save_png).pack(side=tk.LEFT, padx=(8, 0))
        ttk.Button(toolbar, text="Reset Mapping", command=self.reset_mapping).pack(side=tk.LEFT, padx=(8, 0))

        ttk.Label(toolbar, text="View").pack(side=tk.LEFT, padx=(24, 4))
        view_combo = ttk.Combobox(
            toolbar,
            textvariable=self.view_mode_var,
            values=VIEW_MODES,
            state="readonly",
            width=12,
        )
        view_combo.pack(side=tk.LEFT)
        view_combo.bind("<<ComboboxSelected>>", lambda _event: self.refresh_preview())

        for dst_name in ("R", "G", "B", "A"):
            ttk.Label(toolbar, text=f"Out {dst_name}").pack(side=tk.LEFT, padx=(16, 4))
            combo = ttk.Combobox(
                toolbar,
                textvariable=self.map_vars[dst_name],
                values=CHANNEL_SOURCES,
                state="readonly",
                width=4,
            )
            combo.pack(side=tk.LEFT)
            combo.bind("<<ComboboxSelected>>", lambda _event: self.refresh_preview())

        body = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        body.pack(side=tk.TOP, fill=tk.BOTH, expand=True)

        self.main_panel = ttk.Frame(body, padding=8)
        self.side_panel = ttk.Frame(body, padding=8)
        body.add(self.main_panel, weight=4)
        body.add(self.side_panel, weight=1)

        ttk.Label(self.main_panel, text="Preview").pack(side=tk.TOP, anchor=tk.W)
        self.main_label = ttk.Label(self.main_panel, anchor=tk.CENTER)
        self.main_label.pack(side=tk.TOP, fill=tk.BOTH, expand=True)

        ttk.Label(self.side_panel, text="Source Channels").pack(side=tk.TOP, anchor=tk.W)
        self.channel_labels: dict[str, ttk.Label] = {}
        for channel_name in ("R", "G", "B", "A"):
            frame = ttk.Frame(self.side_panel)
            frame.pack(side=tk.TOP, fill=tk.X, pady=(12, 0))
            ttk.Label(frame, text=channel_name).pack(side=tk.TOP, anchor=tk.W)
            label = ttk.Label(frame, anchor=tk.CENTER)
            label.pack(side=tk.TOP, fill=tk.X)
            self.channel_labels[channel_name] = label

        status = ttk.Label(self.root, textvariable=self.status_var, padding=8)
        status.pack(side=tk.BOTTOM, fill=tk.X)

    def load_file(self, path: Path) -> None:
        self.evtex = parse_evtex(path)
        self.status_var.set(self._build_status_text())
        self.refresh_preview()

    def open_file_dialog(self) -> None:
        path_text = filedialog.askopenfilename(
            title="Open EVTEX",
            filetypes=[("EVTEX files", "*.evtex"), ("All files", "*.*")],
        )
        if not path_text:
            return

        try:
            self.load_file(Path(path_text))
        except Exception as exc:
            messagebox.showerror("Failed to open EVTEX", str(exc))

    def save_png(self) -> None:
        if self.evtex is None:
            messagebox.showinfo("No image", "Open an .evtex file first.")
            return

        output_path = filedialog.asksaveasfilename(
            title="Save Remapped PNG",
            defaultextension=".png",
            initialfile=self.evtex.path.with_suffix(".png").name,
            filetypes=[("PNG files", "*.png")],
        )
        if not output_path:
            return

        remapped = remap_channels(self.evtex.rgba, self._current_mapping())
        Image.fromarray(remapped, mode="RGBA").save(output_path)
        self.status_var.set(f"Saved: {output_path}")

    def reset_mapping(self) -> None:
        for dst_name in ("R", "G", "B", "A"):
            self.map_vars[dst_name].set(dst_name)
        self.view_mode_var.set("Composite")
        self.refresh_preview()

    def refresh_preview(self) -> None:
        if self.evtex is None:
            return

        mapping = self._current_mapping()
        remapped = remap_channels(self.evtex.rgba, mapping)
        preview = extract_channel_preview(remapped, self.view_mode_var.get())
        preview = resize_to_fit(preview, (960, 820))
        self.main_photo = ImageTk.PhotoImage(preview)
        self.main_label.configure(image=self.main_photo)

        for channel_name in ("R", "G", "B", "A"):
            channel_image = extract_channel_preview(self.evtex.rgba, channel_name)
            channel_image = resize_to_fit(channel_image, (220, 180))
            photo = ImageTk.PhotoImage(channel_image)
            self.channel_photos[channel_name] = photo
            self.channel_labels[channel_name].configure(image=photo)

        self.status_var.set(self._build_status_text())

    def _current_mapping(self) -> tuple[str, str, str, str]:
        return (
            self.map_vars["R"].get(),
            self.map_vars["G"].get(),
            self.map_vars["B"].get(),
            self.map_vars["A"].get(),
        )

    def _build_status_text(self) -> str:
        if self.evtex is None:
            return "Open an .evtex file to inspect channels."

        usage_name = USAGE_NAMES.get(self.evtex.usage, f"Unknown({self.evtex.usage})")
        mapping = self._current_mapping()
        return (
            f"{self.evtex.path} | {self.evtex.width}x{self.evtex.height} | "
            f"DDS mode={self.evtex.decoded_mode} | sRGB={int(self.evtex.is_srgb)} | "
            f"usage={usage_name} | map RGBA={mapping[0]}{mapping[1]}{mapping[2]}{mapping[3]}"
        )


def main() -> int:
    parser = argparse.ArgumentParser(description="View .evtex textures and remap channels.")
    parser.add_argument("path", nargs="?", help="Optional .evtex file path to open at startup.")
    args = parser.parse_args()

    root = tk.Tk()
    app = EvtexViewerApp(root)

    if args.path:
        try:
            app.load_file(Path(args.path))
        except Exception as exc:
            messagebox.showerror("Failed to open EVTEX", str(exc))
            return 1

    root.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
